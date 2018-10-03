#include "zonotope.h"

/**********************/
/* 1. Meet Lincons    */
/**********************/
zonotope_t* zonotope_meet_lincons_array(elina_manager_t* man, bool destructive, zonotope_t* z, elina_lincons0_array_t* array)
{
    start_timing();
     zonotope_internal_t* pr = zonotope_init_from_manager(man, ELINA_FUNID_MEET_LINCONS_ARRAY);
   //printf("meet start lincons %p %p\n",man,man->internal);
   // zonotope_fprint(stdout,man,z,NULL);
    //elina_lincons0_array_fprint(stdout,array,NULL);
      //  fflush(stdout);
   
    //arg_assert(a && array, abort(););
  
    size_t i = 0;
    zonotope_t* res = destructive ? z : zonotope_copy(man, z);
    bool is_bottom = false;
    elina_interval_t *box = elina_interval_alloc();
    bool* tchange = (bool*)calloc(2*z->dims, sizeof(bool));
    
    size_t kmax = 2; /* specifies the maximum number of iterations */
    /* intervalonly is set to false which means try to improve all dimensions, not only the ones with an interval coefficient */
    
    if (elina_boxize_lincons0_array(res->box, tchange, array, res->box, res->intdim, kmax, false,ELINA_SCALAR_DOUBLE)) {
	    /* there is some inferred bounds */
		
	    for (i=0; i<res->dims; i++) {
		if (tchange[2*i] || tchange[2*i + 1]) {
		    if (elina_interval_is_bottom(res->box[i])) {
			is_bottom = true;
			break;
		    } else if (res->paf[i]->q == NULL) {
			
			zonotope_aff_check_free(pr, res->paf[i]);
			res->paf[i] = zonotope_aff_alloc_init(pr);
			
			if(!elina_scalar_infty(res->box[i]->sup) && !elina_scalar_infty(res->box[i]->inf)){
				
			    zonotope_aff_add_itv(pr, res->paf[i], res->box[i], IN);
				
			} else {
			    elina_interval_set(res->paf[i]->c, res->box[i]);
			}
			
			res->paf[i]->pby++;
		    } 
		}
	    }
	} else {
	    /* nothing change */
	}
	
    //} 
	
    if (!is_bottom) {
	/* texpr -> aff forme */
	
	zonotope_aff_t** aff = (zonotope_aff_t **)malloc(array->size*sizeof(zonotope_aff_t*));
	for (i=0; i<array->size; i++) {
		aff[i]=NULL;
	}
	elina_linexpr0_t* linexpr0 = NULL;
	elina_lincons0_t lincons0;
	elina_interval_t cst;
	for (i=0; i<z->dims; i++) elina_interval_set(res->paf[i]->itv, res->box[i]);
	for (i=0; i<array->size; i++) {
	    aff[i] = zonotope_aff_from_linexpr0(pr, array->p[i].linexpr0, res);
		//printf("after linexpr0: %d %d %p %d \n",i,array->size,aff[i],aff[i]->pby);
		//zonotope_aff_fprint(pr,stdout,aff[i]);
		//fflush(stdout);
	    linexpr0 = elina_linexpr0_from_zonotope(pr,aff[i],res);
        //elina_dimension_t dimension = elina_abstract0_dimension(pr->manNS,z->abs);
        //printf("dimension: %d %d\n",dimension.intdim,dimension.realdim);
        //fflush(stdout);
        
        //elina_dimension_t dimension2 = elina_abstract0_dimension(pr->manNS,res->abs);
        //printf("dimension2: %d %d\n",dimension2.intdim,dimension2.realdim);
        //fflush(stdout);
        
	    if (aff[i]->q != NULL) {
		/* only the centers are involved in this constraint, already treated while updating res->box */
		
		
		/* infer constraints on noise symbols */
		//linexpr0 = zonotope_elina_linexpr0_set_aff(pr, aff[i], res);
		
		lincons0.constyp = array->p[i].constyp;
		lincons0.linexpr0 = linexpr0;
		lincons0.scalar = array->p[i].scalar;
		elina_lincons0_array_t eps_lincons_array;
		eps_lincons_array.size = 1;
		eps_lincons_array.p = &lincons0;
            //printf("epsilon\n");
             //zonotope_aff_fprint(pr, stdout, aff[i]);
            //elina_abstract0_fprint(stdout,pr->manNS,res->abs,NULL);
           // elina_lincons0_array_fprint(stdout,&eps_lincons_array,NULL);
           // fflush(stdout);
            //printf("library before %s %s %d\n",pr->manNS->library,res->abs->man->library,destructive);
           //fflush(stdout);
		elina_abstract0_meet_lincons_array(pr->manNS, true, res->abs, &eps_lincons_array);
            //elina_abstract0_fprint(stdout,pr->manNS,res->abs,NULL);
           // printf("library after %s %p %d\n",pr->manNS->library,res->abs->man,destructive);
           // fflush(stdout);
		if (elina_abstract0_is_bottom(pr->manNS, res->abs)) {
            //elina_linexpr0_free(linexpr0);
		    is_bottom = true;
        elina_linexpr0_free(linexpr0);
		    break;
		}
	    }
	    elina_linexpr0_free(linexpr0);
	}
	/* update res->gamma */
	zonotope_update_noise_symbol_cons_gamma(pr, res);
	
	for (i=0; i<array->size; i++) {
	    /* update the abstract object with the new affine forms */
	    if (array->p[i].constyp == ELINA_CONS_EQ) {
		elina_interval_t *dummy = elina_interval_alloc();
		zonotope_aff_t* tmp, *tmp1;
		size_t j = 0;
		for (j=0; j<res->dims; j++) {
		    if (res->paf[j]->q) {
			
			zonotope_aff_cons_eq_lambda(pr, &dummy, res->paf[j], aff[i], res);
			tmp = zonotope_aff_mul_itv(pr, aff[i], dummy);
			elina_interval_set(res->paf[j]->itv, res->box[j]);
			tmp1 = zonotope_aff_add(pr, tmp, res->paf[j], res);
			zonotope_aff_check_free(pr, res->paf[j]);
			res->paf[j] = tmp1;
			/* update res->box */
			elina_scalar_max(res->box[j]->inf, res->box[j]->inf, res->paf[j]->itv->inf);
			elina_scalar_min(res->box[j]->sup, res->box[j]->sup, res->paf[j]->itv->sup);
			res->paf[j]->pby++;
			zonotope_aff_free(pr, tmp);
		    }
		}
		elina_interval_free(dummy);
	    } else {
		/* do nothing, just update res->box */
		size_t j = 0;
		//zonotope_fprint(stdout,man,res,NULL);
		for (j=0; j<res->dims; j++) {
		    zonotope_aff_bound(pr, box, res->paf[j], res);
		    elina_scalar_max(res->box[j]->inf, res->box[j]->inf, box->inf);
		    elina_scalar_min(res->box[j]->sup, res->box[j]->sup, box->sup);
		}
		//zonotope_fprint(stdout,man,res,NULL);
	    }
		//printf("aff: %d %d %p %d %d\n",i,array->size,aff[i],aff[i]->pby,is_bottom);
		//zonotope_aff_fprint(pr,stdout,aff[i]);
		//fflush(stdout);
	    zonotope_aff_check_free(pr, aff[i]);
	}
	free(aff);
    }
    if (is_bottom) {
	size_t intdim = res->intdim;
	size_t realdim = res->dims - res->intdim;
	zonotope_free(man, res);
	res = zonotope_bottom(man, intdim, realdim);
    }
    
    free(tchange);
    elina_interval_free(box);
   //printf("meet licnons finish %p %p\n",man,man->internal);
   // zonotope_fprint(stdout,man,res,NULL);
   //fflush(stdout);
    
    record_timing(zonotope_meet_lincons_time);
    return res;
}


/************************************************/
/* 2. Join					*/
/************************************************/

zonotope_t* zonotope_join(elina_manager_t* man, bool destructive, zonotope_t* z1, zonotope_t* z2)
    /* TODO destructive not used  */
{
    start_timing();
    size_t i = 0;
    zonotope_internal_t* pr = zonotope_init_from_manager(man, ELINA_FUNID_JOIN);
    if((z1->dims!=z2->dims) || (z1->intdim!=z2->intdim)){
	return NULL;
    }
    //printf("join start\n");
    //zonotope_fprint(stdout,man,z1,NULL);
    //zonotope_fprint(stdout,man,z2,NULL);
   // fflush(stdout);
    zonotope_t* res;
    size_t intdim = z1->intdim;
    size_t realdim = z1->dims - z1->intdim;
    if (zonotope_is_eq(man, z1, z2)) {
	if (destructive) res = z1;
	else res = zonotope_copy(man, z1);
    }
    else if (zonotope_is_top(man, z1) ||  zonotope_is_top(man, z2)) {
	if (destructive) {
	    zonotope_free(man, z1);
	    res = z1 = zonotope_top(man,intdim,realdim);
	} else {
	    res = zonotope_top(man,intdim,realdim);
	}
    } else if (zonotope_is_bottom(man, z1)) {
	if (destructive) {
	    zonotope_free(man, z1);
	    res = z1 = zonotope_copy(man,z2);
	} else {
	    res = zonotope_copy(man,z2);
	}
    } else if (zonotope_is_bottom(man, z2)) {
	if (destructive) res = z1;
	else res = zonotope_copy(man,z1);
    } else {
	/* TODO: destructive not yet supported */
	elina_interval_t *tmp = elina_interval_alloc();
	res = zonotope_alloc(man, intdim, realdim);
	/* update res->box */
	for (i=0; i<(intdim+realdim); i++){
	     elina_scalar_min(res->box[i]->inf,z1->box[i]->inf,z2->box[i]->inf);
	     elina_scalar_max(res->box[i]->sup,z1->box[i]->sup,z2->box[i]->sup);
	}
	
	if ((z1->hypercube && z2->hypercube)) {
	    for (i=0; i<(intdim+realdim); i++) {
		//printf("%d: ",i);
		if (zonotope_aff_is_bottom(pr, z1->paf[i])){
		    res->paf[i] = z2->paf[i];
		}
		else if (zonotope_aff_is_bottom(pr, z2->paf[i])){
		    res->paf[i] = z1->paf[i];
		}
		else if (zonotope_aff_is_top(pr, z1->paf[i]) || zonotope_aff_is_top(pr, z2->paf[i])){
			 res->paf[i] = pr->top;
		}		
		else {
		    if (elina_scalar_infty(z1->box[i]->inf) || elina_scalar_infty(z1->box[i]->sup) 
			|| elina_scalar_infty(z2->box[i]->inf) || elina_scalar_infty(z2->box[i]->sup)) {
			/* Do nothing, the join of concretisations is already done and stored in res->box */
			res->paf[i] = zonotope_aff_alloc_init(pr);
			elina_interval_set(res->paf[i]->c, res->box[i]);
		    } else {
			/* join two affine form expressions */
			elina_interval_set(z1->paf[i]->itv, z1->box[i]);
			elina_interval_set(z2->paf[i]->itv, z2->box[i]);
			res->paf[i] = zonotope_aff_join_constrained6(pr, z1->paf[i], z2->paf[i], z1, z2, res);
		    }
		}
		res->paf[i]->pby++;
	    }

	} else {
	    size_t k = 0;
	    elina_dim_t j = 0;
	    size_t dims1 = zonotope_noise_symbol_cons_get_dimension(pr, z1);
	    size_t dims2 = zonotope_noise_symbol_cons_get_dimension(pr, z2);
	 
	    if (dims1 && dims2) {
		
		size_t dim2 = 0;
		elina_dimchange_t* dimchange2 = elina_dimchange_alloc(0, dims1);
		//elina_dimchange_t* dimchange1 = elina_dimchange_alloc(0, dims2);
		if ((dims1+dims2) > res->size) {
		    res->nsymcons = (elina_dim_t*)realloc(res->nsymcons, (dims1+dims2)*sizeof(elina_dim_t));
		    res->gamma = (elina_interval_t**)realloc(res->gamma, (dims1+dims2)*sizeof(elina_interval_t*));
		    for (k=res->size;k<(dims1+dims2);k++) res->gamma[k] = NULL;
		    res->size = dims1+dims2;
		}
		res->nsymcons = memcpy((void *)res->nsymcons, (const void *)z1->nsymcons, dims1*sizeof(elina_dim_t));
		elina_abstract0_free(pr->manNS, res->abs);
		
		res->abs = elina_abstract0_copy(pr->manNS, z1->abs);
		for (k=0; k<dims2; k++) zonotope_insert_constrained_noise_symbol(pr, &j, z2->nsymcons[k], res);
		for (k=0; k<dims1; k++) {
		    if (!zonotope_noise_symbol_cons_get_dimpos(pr, &j, z1->nsymcons[k], z2)) {
			dimchange2->dim[dim2] = j;
			dim2++;
		    }
		}
		dimchange2->realdim = dim2;
		//size_t dim1 = 0;
		//for (k=0; k<dims2; k++) {
		  //  if (!zonotope_noise_symbol_cons_get_dimpos(pr, &j, z2->nsymcons[k], z1)) {
			//dimchange1->dim[dim1] = j;
			//dim1++;
		    //}
		//}
		//dimchange1->realdim = dim1;
		
		
		/* destructive, without projection (new dimension set to top) */
		elina_abstract0_t * tmp_z2 = elina_abstract0_add_dimensions(pr->manNS, false, z2->abs, dimchange2, false);
		//elina_abstract0_add_dimensions(pr->manNS, true, res->abs, dimchange1, false);
		elina_dimchange_add_invert(dimchange2);
		for (k=0; k<dim2; k++) {
		    zonotope_set_lincons_dim(pr, dimchange2->dim[k]);
		    elina_abstract0_meet_lincons_array(pr->manNS, true, tmp_z2, &pr->moo);
		}

		//elina_dimchange_add_invert(dimchange1); 
		//for (k=0; k<dim1; k++) {
		  //  zonotope_set_lincons_dim(pr, dimchange1->dim[k]);
		    //elina_abstract0_meet_lincons_array(pr->manNS, true, res->abs, &pr->moo);
		//}
		//elina_dimension_t jdim1 = elina_abstract0_dimension(pr->manNS,res->abs);
		//elina_dimension_t jdim2 = elina_abstract0_dimension(pr->manNS,tmp_z2);
		//printf("INPUTS %d %d \n",dim2,jdim1.intdim+jdim1.realdim);
		//elina_abstract0_fprint(stdout,pr->manNS,z1->abs,NULL);
		//elina_abstract0_fprint(stdout,pr->manNS,z2->abs,NULL);
		
		//for(k=0; k < jdim1.intdim+jdim1.realdim; k++){
		//	printf("%u ",res->nsymcons[k]);
		//}
		//printf("\n");
		//fflush(stdout);		
       	        elina_abstract0_join(pr->manNS, true, res->abs, tmp_z2);
		
		
		/* update res->gamma */
		
		zonotope_update_noise_symbol_cons_gamma(pr, res);
		//printf("OUTPUTS %d\n",);
		//elina_abstract0_fprint(stdout,pr->manNS,res->abs,NULL);
		//elina_dimension_t jdim1 = elina_abstract0_dimension(pr->manNS,res->abs);
		//for(k=0; k < zonotope_noise_symbol_cons_get_dimension(pr, res); k++){
		//	printf("%u ",res->nsymcons[k]);
		//}
		//printf("\n");
		//fflush(stdout);	
		//elina_abstract0_free(pr->manNS,tmp_z1);
		elina_abstract0_free(pr->manNS,tmp_z2);
		//elina_abstract0_remove_dimensions(pr->manNS, true, z2->abs, dimchange2);
		//dimchange2->realdim = dims2; 
		elina_dimchange_free(dimchange2);
		//elina_dimchange_free(dimchange1);
		size_t nsymcons_size = zonotope_noise_symbol_cons_get_dimension(pr, res);
		pr->dimtoremove = (elina_dim_t*)realloc(pr->dimtoremove, (nsymcons_size)*sizeof(elina_dim_t));
		memset((void *)pr->dimtoremove, (int)0, nsymcons_size*sizeof(int));
	    } else {
		/* res->abs is a hypercube */
	    }
	    for (i=0; i<(intdim+realdim); i++) {
		if (zonotope_aff_is_bottom(pr, z1->paf[i])){
		    res->paf[i] = z2->paf[i];
		}
		else if (zonotope_aff_is_bottom(pr, z2->paf[i])){
		    res->paf[i] = z1->paf[i];
	        }
		else if (zonotope_aff_is_top(pr, z1->paf[i]) || zonotope_aff_is_top(pr, z2->paf[i])){
		    res->paf[i] = pr->top;
		}
		else if (zonotope_aff_is_eq(pr, z1->paf[i], z2->paf[i])){
		    res->paf[i] = z1->paf[i];
		}
		else {
		    if (elina_scalar_infty(z1->box[i]->inf) || elina_scalar_infty(z1->box[i]->sup)
			 || elina_scalar_infty(z2->box[i]->inf) || elina_scalar_infty(z2->box[i]->sup)) {
			/* Do nothing, the join of concretisations is already done and stored in res->box */
			res->paf[i] = zonotope_aff_alloc_init(pr);
			elina_interval_set(res->paf[i]->c, res->box[i]);
		    } else {
			/* join two affine form expressions */
			elina_interval_set(z1->paf[i]->itv, z1->box[i]);
			elina_interval_set(z2->paf[i]->itv, z2->box[i]);
			res->paf[i] = zonotope_aff_join_constrained6(pr, z1->paf[i], z2->paf[i], z1, z2, res);
		    }
		}
		res->paf[i]->pby++;
	    }

	    man->result.flag_best = false;
	    man->result.flag_exact = false;
	}
	
	man->result.flag_best = true;
	man->result.flag_exact = false;
	elina_interval_free(tmp);
    }
    man->result.flag_best = true;
    man->result.flag_exact = true;
    //printf("Join output\n");
    //zonotope_fprint(stdout,man,res,NULL);
    //fflush(stdout);
    //size_t dims = zonotope_noise_symbol_cons_get_dimension(pr, res);
	//printf("join output dims %d\n",dims);
	//for(size_t k=0; k < dims; k++){
	//	printf("%u ",res->nsymcons[k]);
	//}
	//printf("\n");
	//fflush(stdout);
    record_timing(zonotope_join_time);
    return res;
}


