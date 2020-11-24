#include"requestreceiver.h"

RequestReceiver::RequestReceiver() {
    head = NULL;
    body = NULL;
    next = false;
}

RequestReceiver::~RequestReceiver() {
    if (head != NULL) {
        delete[] head;
    }

    if (body != NULL) {
        delete[] body;
    }
}




