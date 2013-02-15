// double-linked list macros

#define list_insert(head,add) \
	do { \
		(add)->next = head; \
		if (head) { \
			(add)->prev = (head)->prev; \
			(head)->prev = (add); \
		} else { \
			(add)->prev = (add); \
		} \
		(head) = (add); \
	} while (0)

#define list_append(head,add) \
	do { \
		if (head) { \
			(add)->prev = (head)->prev; \
			(head)->prev->next = (add); \
			(head)->prev = (add); \
			(add)->next = NULL; \
		} else { \
			(head)=(add); \
			(head)->prev = (head); \
			(head)->next = NULL; \
		} \
	} while (0)

#define list_delete(head,del) \
	do { \
		assert((del)->prev != NULL); \
		if ((del)->prev == (del)) { \
			(head)=NULL; \
		} else if ((del)==(head)) { \
			(del)->next->prev = (del)->prev; \
			(head) = (del)->next; \
		} else { \
			(del)->prev->next = (del)->next; \
			if ((del)->next) { \
				(del)->next->prev = (del)->prev; \
			} else { \
				(head)->prev = (del)->prev; \
			} \
		} \
	} while (0)

#define list_for_each(head,el) \
	for(el=head;el;el=(el)->next)

#define list_for_each_safe(head,el,tmp) \
	for((el)=(head);(el) && (tmp = (el)->next, 1); (el) = tmp)

