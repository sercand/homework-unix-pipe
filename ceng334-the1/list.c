//
//  list.c
//  ceng334-the1
//
//  Created by Sercan Değirmenci on 27/03/16.
//  Copyright © 2016 Otsimo. All rights reserved.
//

#include "list.h"
#include <stdlib.h>

typedef struct list_node
{
    int _id;
    int _in;
    int _out;
    struct list_node *next;
}list_node;

list_node* head = NULL;

list_node * llsearch(int num)
{
    list_node *temp;
    temp = head;
    while(temp != NULL)
    {
        if(temp->_id == num){
            return temp;
        }
        temp = temp->next;
    }
    return NULL;
}

void lladd(int data,int _in, int _out){
    list_node *new_node,*temp;
    
    new_node = llsearch(data);
    if(new_node != NULL){
        new_node->_in += _in;
        new_node->_out += _out;
        return;
    }
    
    new_node = (list_node *)malloc(sizeof(list_node));
    
    if(new_node == NULL){
        printf("Failed to Allocate Memory");
        exit(5);
    }
    new_node->_id = data;
    new_node->_in = _in;
    new_node->_out = _out;
    new_node->next=NULL;
    
    if(head==NULL)
    {
        head = new_node;
    }
    else
    {
        temp = head;
        while(temp->next != NULL)
        {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

int llvalid(int* size){
    int a = 0;
    list_node *temp;
    temp = head;
    while(temp != NULL)
    {
        a += 1;
        //WTF
        if(!((temp->_in >= 1 && temp->_out >= 1) || (temp->_in == 0 && temp->_out == 0))){
            return 0;
        }
        temp = temp->next;
    }
    *size = a;
    return 1;
}

void llclear(){
    list_node *temp2,*temp;
    temp = head;
    
    while(temp!=NULL && temp->next != NULL)
    {
        temp2 = temp->next;
        
        free(temp);
        
        temp = temp2;
    }
    head = NULL;
}
