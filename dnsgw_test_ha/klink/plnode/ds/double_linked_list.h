/* 
 * File:   doubleLinkedList.h
 * Author: mfbari
 *
 * Created on November 28, 2012, 9:00 PM
 */

#ifndef DOUBLELINKEDLIST_H
#define	DOUBLELINKEDLIST_H

#include <cstdlib>
#include <cstring>

#include "overlay_id.h"
#include "host_address.h"

using namespace std;

struct DLLNode {
        OverlayID key;
        HostAddress value;
        DLLNode *prev, *next;

        DLLNode() {
                prev = next = NULL;
        }

        DLLNode(OverlayID &key, HostAddress & value) {
                this->key = key;
                this->value = value;
                prev = next = NULL;
        }
};

class DoublyLinkedList {
        DLLNode *head, *tail;
public:

        DoublyLinkedList() {
                head = tail = NULL;

        }

        ~DoublyLinkedList() {
                DLLNode *temp1 = tail, *temp2;
                while (temp1 != NULL) {
                        temp2 = temp1;
                        temp1 = temp1->prev;
                        delete temp2;
                }
                head = tail = NULL;
        }

        bool contains(OverlayID &key) {
                DLLNode *temp = tail;
                while (temp != NULL) {
                        if (tail->key == key)
                                return true;
                        temp = temp->prev;
                }
                return false;
        }

        void append2Head(DLLNode *node) {
                if (head == NULL) {
                        head = tail = node;
                        head->prev = tail->next = NULL;
                } else {
                        node->next = head;
                        head->prev = node;
                        head = node;
                        head->prev = NULL;
                }
        }

        void append2Head(OverlayID &key, HostAddress &value) {
                DLLNode *node = new DLLNode(key, value);
                append2Head(node);
        }

        void append2Tail(DLLNode *node) {
                if (tail == NULL) {
                        head = tail = node;
                        head->prev = tail->next = NULL;
                } else {
                        node->prev = tail;
                        tail->next = node;
                        tail = node;
                        tail->next = NULL;
                }
        }

        void append2Tail(OverlayID &key, HostAddress &value) {
                DLLNode *node = new DLLNode(key, value);
                append2Tail(node);
        }

        DLLNode* getTail() {
                return tail;
        }

        void removeTail() {
                //only one node
                if (tail == head) {
                        delete tail;
                        head = tail = NULL;
                } else {
                        DLLNode *temp = tail;
                        tail = tail->prev;
                        tail->next = NULL;
                        temp->prev = NULL;
                        //delete temp;
                }
        }

        DLLNode* getHead() {
                return head;
        }

        DLLNode* extract(DLLNode *node) {
                if (head == NULL || tail == NULL) {
                        return NULL;
                } else if (head == node && tail == node) {
                        head = tail = NULL;
                } else if (head == node) {
                        head = node->next;
                        head->prev = NULL;
                } else if (tail == node) {
                        tail = node->prev;
                        tail->next = NULL;
                } else {
                        node->prev->next = node->next;
                        node->next->prev = node->prev;
                }
                node->next = node->prev = NULL;
                return node;
        }

        void remove(DLLNode *node) {
                delete extract(node);
        }

        void move2Head(DLLNode *node) {
                if (head == NULL || tail == NULL)
                        return;
                if (head != node) {
                        //append2Head(extract(node));
                        if (tail == node) {
                                tail = tail->prev;
                                tail->next = NULL;
                                puts("tail == node");
                        } else {
                                node->prev->next = node->next;
                                node->next->prev = node->prev;
                                puts("tail != node");
                        }
                        node->next = head;
                        head->prev = node;
                        head = node;
                        head->prev = NULL;
                }
        }

        void printNodesReverse() {
                DLLNode *temp = tail;
                cout << "Nodes in reverse order :" << endl;
                while (temp != NULL) {
                        cout << "< " << temp->key.GetOverlay_id() << ", "
                                << temp->value.GetHostName() << " > ";
                        temp = temp->prev;
                }
                cout << endl;
        }

        void printNodesForward() {
                DLLNode *temp = head;
                cout << "Nodes in forward order:" << endl;
                while (temp != NULL) {
                        cout << "< " << temp->key.GetOverlay_id() << ", "
                                << temp->value.GetHostName() << " > ";
                        temp = temp->next;
                }
                cout << endl;
        }

        char* toString() {
                int size = getStringSize(), index = 0;
                char* result = new char[size + 1];
                DLLNode *temp = head;
                while (temp != NULL) {
                        sprintf(result + index, "%s --> %s<br/>", temp->key.toString(), temp->value.toString());
                        index += temp->key.getStringSize() + 5 + temp->value.getStringSize() + 5;
                        temp = temp->next;
                        //puts("AMISTUCK");
                }
                result[size] = '\0';
                return result;
        }

        int getStringSize() {
                //puts("at-least i am here");
                int size = 0;
                DLLNode *temp = head;
                while (temp != NULL) {
                        size += temp->key.getStringSize() + 5 + temp->value.getStringSize() + 5;
                        temp = temp->next;
                        //puts("AMISTUCK in get size?");
                }
                return size;
        }
};

#endif	/* DOUBLELINKEDLIST_H */

