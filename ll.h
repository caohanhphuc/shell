#include "ll.c"

int sizearg(char** arg);
void printArg(char**arg, int argsize);
void printListHelper(struct node* ptr);
void printList();
void printReverse();
int maxIndex();
void insertFirst(int key, int data, int status, char** cmd, int argsize, struct termios term);
struct node* deleteFirst();
bool isEmpty();
int length();
struct node* find(int key);
struct node* resumeBG();
struct node* resumeFG();
struct node* delete(int key);
struct node* deletePid(int data);
