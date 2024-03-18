#pragma once
#ifndef LINKEDLIST_HEADER
#define LINKEDLIST_HEADER


typedef struct node {
    int data;
    struct node* next;
} Node;

typedef struct list {
    Node* head;
} List;

//typedef struct node Node;

//typedef struct list List;

List* makelist();
void addNode(int data, List* list);
void deleteNode(int data, List* list, bool FreeData);
void display(List* list);
void reverse(List* list);
void reverse_using_two_pointers(List* list);
void destroyList(List* list, bool FreeData);

#endif
