/* 
   Taken from https://www.tutorialspoint.com/data_structures_algorithms/linked_list_program_in_c.htm 

 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>

typedef enum {EXC, BGN, BGR, FGR, KLL, LST, EXT, SUS} jobaction;

struct node {
  int data; //process id
  int key; //jobkey
  int status;
  char** cmd;
  int argsize;
  struct termios term;
  struct node *next;
};

struct node *head = NULL;
struct node *current = NULL;

//display the list


void printArg(char** arg, int argsize){
  for (int i = 0; i < argsize; i++){
    if (arg[i] != NULL){
      printf("%s ", arg[i]);
    }
  } 
}


void printListHelper(struct node* ptr){
  if (ptr == NULL)
       return;
  printListHelper(ptr->next);
  char** command = ptr->cmd;
  printf("[%d]\t", ptr->key);
  if (ptr->status == BGR){
    printf("Running\t\t");
  } else if (ptr->status == SUS){
    printf("Stopped\t\t");
  } else {
    printf("Status %d\n", ptr->status);
  }
  printArg(command, ptr->argsize);
  printf("\n");
}

void printList(){
  printListHelper(head);
}

void printReverse() {
   struct node *ptr = head;
	
   //start from the beginning
   while(ptr != NULL) {
     char** command = ptr->cmd;
     printf("[%d]\t", ptr->key);
     if (ptr->status == BGR){
       printf("Running\t\t");
     } else if (ptr->status == SUS){
       printf("Stopped\t\t");
     }
     printArg(command, ptr->argsize);
     printf("\n");
     ptr = ptr->next;
   } 
}

int maxIndex(){
  int idx;
  struct node* curr = head;
  
  if(head == NULL) {
    return 0;
  }
  idx = head->key;
  while(curr->next != NULL) {
    if(curr->key > idx) {
      idx = curr->key;
    }
    curr = curr->next;
  }      
  return idx;
}

//insert link at the first location
void insertFirst(int key, int data, int status, char** cmd, int argsize, struct termios term) {
   //create a link
   struct node *link = (struct node*) malloc(sizeof(struct node));
	
   link->key = key;
   link->data = data;
   link->status = status;
   link->cmd = cmd;
   link->argsize = argsize;
   link->term = term;
	
   //point it to old first node
   link->next = head;
	
   //point first to new first node
   head = link;
}

//delete first item
struct node* deleteFirst() {

   //save reference to first link
   struct node *tempLink = head;
	
   //mark next to first link as first 
   head = head->next;
	
   //return the deleted link
   return tempLink;
}

//is list empty
bool isEmpty() {
   return head == NULL;
}

int length() {
   int length = 0;
   struct node *current;
	
   for(current = head; current != NULL; current = current->next) {
      length++;
   }
   
   return length;
}

//find a link with given key
struct node* find(int key) {

   //start from the first link
   struct node* current = head;

   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->key != key) {
	
      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
	
   //if data found, return the current Link
   return current;
}

struct node* resumeFG(){
  struct node* current = head;

  if (head == NULL) return NULL;

  while (current->status != BGR && current->status != SUS){
    if (current->next == NULL){
      return NULL;
    } else {
      current = current->next;
    }
  }
  return current;
}

struct node* resumeBG(){
  struct node* current = head;

  if (head == NULL) return NULL;

  while (current->status != SUS){
    if (current->next == NULL){
      return NULL;
    } else {
      current = current->next;
    }
  }
  return current;
}  

//delete a link with given key
struct node* deletePid(int data) {

   //start from the first link
   struct node* current = head;
   struct node* previous = NULL;
	
   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->data != data) {

      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
	
   return current;
}

struct node* delete(int key) {

   //start from the first link
   struct node* current = head;
   struct node* previous = NULL;
	
   //if list is empty
   if(head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->key != key) {

      //if it is last node
      if(current->next == NULL) {
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == head) {
      //change first to point to next link
      head = head->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
	
   return current;
}



/* 
void sort() {

   int i, j, k, tempKey, tempData;
   struct node *current;
   struct node *next;
	
   int size = length();
   k = size ;
	
   for ( i = 0 ; i < size - 1 ; i++, k-- ) {
      current = head;
      next = head->next;
		
      for ( j = 1 ; j < k ; j++ ) {   
		
         if ( current->data > next->data ) {
            tempData = current->data;
            current->data = next->data;
            next->data = tempData;

            tempKey = current->key;
            current->key = next->key;
            next->key = tempKey;
         }
			
         current = current->next;
         next = next->next;
      }
   }   
}

void reverse(struct node** head_ref) {
   struct node* prev   = NULL;
   struct node* current = *head_ref;
   struct node* next;
	
   while (current != NULL) {
      next  = current->next;
      current->next = prev;   
      prev = current;
      current = next;
   }
	
   *head_ref = prev;
}
*/

/*
main() {
   insertFirst(1,10);
   insertFirst(2,20);
   insertFirst(3,30);
   insertFirst(4,1);
   insertFirst(5,40);
   insertFirst(6,56); 

   printf("Original List: "); 
	
   //print list
   printList();

   while(!isEmpty()) {            
      struct node *temp = deleteFirst();
      printf("\nDeleted value:");
      printf("(%d,%d) ",temp->key,temp->data);
   }  
	
   printf("\nList after deleting all items: ");
   printList();
   insertFirst(1,10);
   insertFirst(2,20);
   insertFirst(3,30);
   insertFirst(4,1);
   insertFirst(5,40);
   insertFirst(6,56);
   
   printf("\nRestored List: ");
   printList();
   printf("\n");  

   struct node *foundLink = find(4);
	
   if(foundLink != NULL) {
      printf("Element found: ");
      printf("(%d,%d) ",foundLink->key,foundLink->data);
      printf("\n");  
   } else {
      printf("Element not found.");
   }

   delete(4);
   printf("List after deleting an item: ");
   printList();
   printf("\n");
   foundLink = find(4);
	
   if(foundLink != NULL) {
      printf("Element found: ");
      printf("(%d,%d) ",foundLink->key,foundLink->data);
      printf("\n");
   } else {
      printf("Element not found.");
   }
	
   printf("\n");
   sort();
	
   printf("List after sorting the data: ");
   printList();
	
   reverse(&head);
   printf("\nList after reversing the data: ");
   printList();
} */
