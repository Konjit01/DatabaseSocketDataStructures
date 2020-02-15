/***********************************************************************
** FILENAME: dplist.c                                                 **
** Author: Konjit Sileshi                                             **
** Implementation of doubly linkedlist                                **
** Resources: Proffesors starting and academic codes                  **
***********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									\
		do {											\
			printf("\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	\
			printf(__VA_ARGS__);								\
		} while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,dplist_errno_value,...)\
	do {						\
		if ((condition))			\
		{					\
		  dplist_errno = dplist_errno_value;	\
		  DEBUG_PRINTF(#condition "failed");	\
		  return __VA_ARGS__;			\
		}					\
		dplist_errno = DPLIST_NO_ERROR;			\
	} while(0)

#define FREE(p) do{ free(p); p=NULL;} while(0)
/*
 * The real definition of struct list / struct node
 */
int dplist_errno;

 struct dplist_node {
  dplist_node_t *prev, *next;
  void * element;
};

struct dplist {
  dplist_node_t *head;
  //struct dplist_node *head;
   int size;
  //Function pointers declaration.
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
 };

//CREATES AND INIT
dplist_t *dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			 ){
  dplist_t *list;
  list = (dplist_t *)malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER((list==NULL),DPLIST_MEMORY_ERROR,NULL);
  list->head = NULL;  
  list->size = 0;
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare;  
  return list;
}
//FREE ALL
void dpl_free(dplist_t ** list){
    DPLIST_ERR_HANDLER((list==NULL),DPLIST_INVALID_ERROR);
	DPLIST_ERR_HANDLER((*list==NULL),DPLIST_INVALID_ERROR);
	int i = (*list) ->size-1;
	while((*list)->size !=0 || i>0){
		dpl_remove_at_index(*list, i, true);
		i--;
	}
	free(*list);
	*list=NULL;
}
//INSERT AT INDEX
dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
  dplist_node_t * ref_at_index, * list_node;
  DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, list);
  list_node = (dplist_node_t*)malloc(sizeof(dplist_node_t));
  DPLIST_ERR_HANDLER((list_node==NULL),DPLIST_MEMORY_ERROR,NULL);
  
  if(insert_copy == true){
     list_node->element = list->element_copy((element));
	}
  else{
	 list_node->element = element;  
  }
  if(list ->head == NULL){
	  list_node ->prev =NULL;
	  list_node ->next = NULL;
	  list->head = list_node;
	  
	  list->size++;
	  return list;
  }
  
  else if(index<=0){
	  list_node->prev= NULL;
	  list_node->next = list->head;
	  list->head->prev = list_node;
	  list->head = list_node;
	  list->size ++;
	  return list;
	  
  }
  else{
	  ref_at_index = dpl_get_reference_at_index(list, index); //temp
	  assert(ref_at_index!=NULL);
	  if(index <dpl_size(list)){
		  list_node->prev = ref_at_index->prev;
		  list_node->next = ref_at_index;
		  ref_at_index->prev->next = list_node;
		  ref_at_index->prev=list_node;
		  list->size++;
	  }else{
		  assert(ref_at_index->next==NULL);
		  list_node->next=NULL;
		  list_node->prev=ref_at_index;
		  ref_at_index->next=list_node;
		   list->size++;
		 }
   }
 return list;
 }
//REMOVE AT AN INDEX
 dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool element_free)
 {
    DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	if(index <= 0){
		dplist_node_t *temp = list->head;
		while(temp != NULL){
			if(temp->next == NULL){
				if(element_free==true){
					list->element_free(&(temp->element));
				}
				else{
				free((temp->element));
				(temp->element)=NULL;}
				free(temp);
				temp = NULL;
				list->size --;
				return list;
			}
			else{
				list->head = temp->next;
				list->head ->prev = NULL;
				if(element_free==true){
					list->element_free(&(temp->element));
				}else{
				free((temp->element));
				(temp->element)=NULL;}
				free(temp);
				temp = NULL;
				list->size --;
				return list;
			}
		}
	}
	if(index >= (list->size)-1){
		dplist_node_t *temp = list->head;
		while(temp->next != NULL){
			temp =temp->next;
		}
		
		if(temp->prev == NULL){
			if(element_free==true){
				list->element_free(&(temp->element));
			}else{
				free((temp->element));
				(temp->element)=NULL;
			}
			free(temp);
			temp = NULL;
			list->size --;
			return list;
		}
		else{
			temp = temp->prev;
			if(element_free==true){
				list->element_free(&(temp->next->element));
			}else{
				free((temp->next->element));
				(temp->next->element)=NULL;
			}
			free(temp->next);
			temp->next = NULL;
			list->size --;
			return list;
		}
	}
	int i = 1;
	dplist_node_t *temp = list->head;
	while( i < index){
		temp = temp->next;
		i++;
	}
	
	dplist_node_t* dummy= temp->next->next;
	if(element_free==true){
		list->element_free(&(temp->next->element));
	}else{
		free((temp->next->element));
		temp->next->element=NULL;
	}
	free(temp->next);
	temp->next =dummy;
	dummy->prev = temp;
	
	list->size --;
	return list;
}
//SIZE		 
int dpl_size( dplist_t * list ){  
    DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, -1);
	return list->size;
}
//REFERENCE OF INDEX
dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
	//printf("\nList at the ref: %p\n", list);
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	if(list->size == 0){
		return NULL;
	}
	if(index<=0){
		return list->head;
	}
	else if(index>=(list->size)){
		dplist_node_t * temp = list->head;
		while(temp->next!=NULL){
			temp = temp->next;
		}
		return temp;
		
	}else{
	int track = 0;
	dplist_node_t *temp = list ->head;
	while(track<index){
		temp = temp->next;
		track ++;
	}
    return temp;
	}
}
//GET ELEMENT AT INDEX
void * dpl_get_element_at_index( dplist_t * list, int index )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	if(list->size == 0){
		return (void*)NULL;
	}
	dplist_node_t * ref_at_index = dpl_get_reference_at_index(list, index);
    return ref_at_index->element;
}
//GET INDEX  OF ELEMENT 
int dpl_get_index_of_element( dplist_t * list, void * element ){
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, -1);
	int index = 0;
	dplist_node_t *temp = list->head;
	while(temp!=NULL){
		if(list->element_compare(element, temp->element)==0){
			return index;
		}
		temp = temp->next;
		index++;
	}
return -1;
}


                                //EXTRAS
//GET REFERENCE TO THE FIRST ELEMENT -WORKS FINE 
dplist_node_t * dpl_get_first_reference( dplist_t * list )
{ 
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	if(list->size == 0){
		return NULL;
	}else{
		return list->head;
	}
}
//GET REFERENCE TO THE LAST ELEMENT WORKS FINE
dplist_node_t * dpl_get_last_reference( dplist_t * list )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	if(list->size == 0){
		return NULL;
	}
	else{
	dplist_node_t * temp = list->head;
	while(temp->next!=NULL){
		temp = temp->next;
	    }
	return temp;
		
	}
}
//GET REFERENCE TO THE NEXT -WORKS FINE
dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	DPLIST_ERR_HANDLER((reference==NULL), DPLIST_INVALID_ERROR, NULL);
	
	dplist_node_t *temp = list->head;
	while(temp!=NULL){
	if(list->element_compare(reference,temp)==0){
			return temp->next;
	}
	temp = temp->next;
	}
	return NULL;
}
//GET REFERENCE TO THE PREVIOUS - WORKS FINE
dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	DPLIST_ERR_HANDLER((reference==NULL), DPLIST_INVALID_ERROR, NULL);
	
	dplist_node_t *temp = list->head;
	while(temp!=NULL){
		/* if(list->element_compare(reference,  temp)==0){
			return temp->prev;
		} */
		temp = temp->next;
	}
	return NULL;
}
//GET ELEMENT AT REFERENCE -WORKS FINE
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	dplist_node_t* temp = list->head;
	
	if(list->size == 0){
		return NULL;
	}
	if(reference==NULL){
		return dpl_get_last_reference( list )->element;
	}else{
	    while(temp!=NULL){
			if(reference==temp){
				int index;
				index = dpl_get_index_of_reference( list, temp );
				return dpl_get_element_at_index( list,  index );
			}
			else{
				temp=temp->next;
			}
		} 
		return NULL;
	}
	
}
//GET REFERENCE OF ELEMENT
dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
    if(list->size == 0){
		return NULL;
	}
	dplist_node_t *temp = list->head;
	while(temp!=NULL){
	if(temp->element==NULL){
			temp = temp->next;
	}else{
			return temp;
	    }
	}
	return NULL;
}
//RETURNS INDEX OF A GIVEN REFERENCE -WORKS FINE
int dpl_get_index_of_reference( dplist_t * list, dplist_node_t * reference )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, -1);
	dplist_node_t* temp = list->head;
	if(list->size == 0){
		return -1;
	}
	if(reference==NULL){
		temp=dpl_get_last_reference(list);
		return dpl_get_index_of_element(list, (void*)(temp->element) );
	}

	while(temp!=NULL){
		if(temp==reference) {
			return dpl_get_index_of_element(list, (void*)(temp->element) );
		}else{
			 temp = temp->next;
		}
	}
	return -1;
}
//INSERT AT A REFERENCE-WORKS FINE
dplist_t * dpl_insert_at_reference( dplist_t * list, void * element, dplist_node_t * reference, bool insert_copy )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
    dplist_node_t* temp = list->head;
	if(temp==NULL){
		return dpl_insert_at_index(list, element, 0, false);
	}
	int index1;
	if(reference == NULL){
		temp = dpl_get_last_reference( list );
		index1=dpl_get_index_of_reference(list, temp);
		return dpl_insert_at_index(list, element,  (index1+1), false);
    }else {
		while(temp!=NULL){
			if(temp==reference){
				index1=dpl_get_index_of_reference(list, temp);
				return dpl_insert_at_index(list, element,  (index1), false);
			}else{
				temp=temp->next;
		    }
	    } return list;
    }
}
//INSERTION SORTED-WORKS FINE
dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);

	dplist_node_t* temp = list->head;
     int size;
	 size = dpl_size(list);

	
 
		while(temp!=NULL){
			int value= list->element_compare(element, temp->element);
			if(value==1){
				temp=temp->next;
			}
			if(value==-1 ){
				int index;
				index=dpl_get_index_of_reference(list, temp );
				printf("At line insert sorted:  %20d", index);
				return  dpl_insert_at_index(list, element, index, false);
				
			}
			if(value == 0){
				int index;
				index=dpl_get_index_of_reference(list, temp );
				return  dpl_insert_at_index(list, element, index+1, false);
			}

	}
   return  dpl_insert_at_index(list, element, size, false);
} 
//REMOVES AT A REFERENCE 
dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	int index;
	dplist_node_t* temp = list->head;
	if(list->size == 0){
		return list;
	}
	if(reference==NULL){
		temp=dpl_get_last_reference(list);
		index = dpl_get_index_of_reference( list, temp );
	    return dpl_remove_at_index( list, (index), false);
	}else {
	    while(temp!=NULL){
			if(temp==reference){
				index = dpl_get_index_of_reference( list, temp );
				return dpl_remove_at_index( list, (index), false);
			}else{
				temp=temp->next;
			}
		}	
		return list;
	}
}
//REMOVE ELEMENT AT INDEX
dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element )
{
	DPLIST_ERR_HANDLER((list==NULL), DPLIST_INVALID_ERROR, NULL);
	int index;
	index = dpl_get_index_of_element(  list, element );
	if(index==-1){
		return list;
	}else{
		return dpl_remove_at_index( list, (index), false);
	}
}




