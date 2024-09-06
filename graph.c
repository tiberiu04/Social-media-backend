#include <stdio.h>
#include "graph.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define DIE(assertion, call_description)            \
	do                                              \
	{                                               \
		if (assertion)                              \
		{                                           \
			fprintf(stderr, "(%s, %d): ", __FILE__, \
					__LINE__);                      \
			perror(call_description);               \
			exit(errno);                            \
		}                                           \
	} while (0)

#define MAX_QUEUE_SIZE 100

static inline int min(int x, int y)
{
	return x < y ? x : y;
}

/* Helper data structures functions */
linked_list_t *
ll_create(unsigned int data_size)
{
	linked_list_t *ll = calloc(1, sizeof(*ll));
	DIE(!ll, "calloc list");

	ll->data_size = data_size;

	return ll;
}

static ll_node_t *
get_nth_node(linked_list_t *list, unsigned int n)
{
	unsigned int len = list->size - 1;
	unsigned int i;
	ll_node_t *node = list->head;

	n = min(n, len);

	for (i = 0; i < n; ++i)
		node = node->next;

	return node;
}

static ll_node_t *
create_node(const void *new_data, unsigned int data_size)
{
	ll_node_t *node = calloc(1, sizeof(*node));
	DIE(!node, "calloc node");

	node->data = malloc(data_size);
	DIE(!node->data, "malloc data");

	memcpy(node->data, new_data, data_size);

	return node;
}

static ll_node_t *find_node(linked_list_t *ll, int node, unsigned int *pos)
{
	ll_node_t *crt = ll->head;
	unsigned int i;

	for (i = 0; i != ll->size; ++i)
	{
		if (node == *(int *)crt->data)
		{
			*pos = i;
			return crt;
		}

		crt = crt->next;
	}

	return NULL;
}

void ll_add_nth_node(linked_list_t *list, unsigned int n, const void *new_data)
{
	ll_node_t *new_node, *prev_node;

	if (!list)
		return;

	new_node = create_node(new_data, list->data_size);

	if (!n || !list->size)
	{
		new_node->next = list->head;
		list->head = new_node;
	} else {
		prev_node = get_nth_node(list, n - 1);
		new_node->next = prev_node->next;
		prev_node->next = new_node;
	}

	++list->size;
}

ll_node_t *
ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *prev_node, *removed_node;

	if (!list || !list->size)
		return NULL;

	if (!n)
	{
		removed_node = list->head;
		list->head = removed_node->next;
		removed_node->next = NULL;
	} else {
		prev_node = get_nth_node(list, n - 1);
		removed_node = prev_node->next;
		prev_node->next = removed_node->next;
		removed_node->next = NULL;
	}

	--list->size;

	return removed_node;
}

unsigned int
ll_get_size(linked_list_t *list)
{
	return !list ? 0 : list->size;
}

void ll_free(linked_list_t **pp_list)
{
	ll_node_t *node;

	if (!pp_list || !*pp_list)
		return;

	while ((*pp_list)->size)
	{
		node = ll_remove_nth_node(*pp_list, 0);
		free(node->data);
		free(node);
	}

	free(*pp_list);
	*pp_list = NULL;
}

void ll_print_int(linked_list_t *list)
{
	ll_node_t *node = list->head;

	for (; node; node = node->next)
		printf("%d ", *(int *)node->data);
	printf("\n");
}

void ll_print_string(linked_list_t *list)
{
	ll_node_t *node = list->head;

	for (; node; node = node->next)
		printf("%s ", (char *)node->data);
	printf("\n");
}

queue_t *
q_create(unsigned int data_size, unsigned int max_size)
{
	queue_t *q = calloc(1, sizeof(*q));
	DIE(!q, "calloc queue failed");

	q->data_size = data_size;
	q->max_size = max_size;

	q->buff = malloc(max_size * sizeof(*q->buff));
	DIE(!q->buff, "malloc buffer failed");

	return q;
}

unsigned int
q_get_size(queue_t *q)
{
	return !q ? 0 : q->size;
}

unsigned int
q_is_empty(queue_t *q)
{
	return !q ? 1 : !q->size;
}

void *
q_front(queue_t *q)
{
	if (!q || !q->size)
		return NULL;

	return q->buff[q->read_idx];
}

int q_dequeue(queue_t *q)
{
	if (!q || !q->size)
		return 0;

	free(q->buff[q->read_idx]);

	q->read_idx = (q->read_idx + 1) % q->max_size;
	--q->size;
	return 1;
}

int q_enqueue(queue_t *q, void *new_data)
{
	void *data;
	if (!q || q->size == q->max_size)
		return 0;

	data = malloc(q->data_size);
	DIE(!data, "malloc data failed");
	memcpy(data, new_data, q->data_size);

	q->buff[q->write_idx] = data;
	q->write_idx = (q->write_idx + 1) % q->max_size;
	++q->size;

	return 1;
}

void q_clear(queue_t *q)
{
	unsigned int i;
	if (!q || !q->size)
		return;

	for (i = q->read_idx; i != q->write_idx; i = (i + 1) % q->max_size)
		free(q->buff[i]);

	q->read_idx = 0;
	q->write_idx = 0;
	q->size = 0;
}

void q_free(queue_t *q)
{
	if (!q)
		return;

	q_clear(q);
	free(q->buff);
	free(q);
}

static int is_node_in_graph(int n, int nodes)
{
	return n >= 0 && n < nodes;
}

list_graph_t *
lg_create(int nodes)
{
	int i;

	list_graph_t *g = malloc(sizeof(*g));
	DIE(!g, "malloc graph failed");

	g->neighbors = malloc(nodes * sizeof(*g->neighbors));
	DIE(!g->neighbors, "malloc neighbours failed");

	for (i = 0; i != nodes; ++i)
		g->neighbors[i] = ll_create(sizeof(int));

	g->nodes = nodes;

	return g;
}

void lg_add_edge(list_graph_t *graph, int src, int dest)
{
	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return;

	linked_list_t *neighbor_list = graph->neighbors[src];
	ll_node_t *current = neighbor_list->head;
	int position = 0;

	while (current && *((int *)current->data) < dest) {
		current = current->next;
		position++;
	}

	if (!current)
		position = graph->neighbors[src]->size;

	ll_add_nth_node(graph->neighbors[src], position, &dest);
}

int lg_has_edge(list_graph_t *graph, int src, int dest)
{
	unsigned int pos;

	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return 0;

	ll_node_t *found_node = find_node(graph->neighbors[src], dest, &pos);
	if (!found_node)
		return 0;
	else
		return 1;
}

linked_list_t *
lg_get_neighbours(list_graph_t *graph, int node)
{
	if (!graph || !graph->neighbors || !is_node_in_graph(node, graph->nodes))
		return NULL;

	return graph->neighbors[node];
}

void lg_remove_edge(list_graph_t *graph, int src, int dest)
{
	unsigned int pos;

	if (!graph || !graph->neighbors || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return;

	if (!find_node(graph->neighbors[src], dest, &pos))
		return;

	ll_node_t *node = ll_remove_nth_node(graph->neighbors[src], pos);
	free(node->data);
	free(node);
}

void lg_free(list_graph_t *graph)
{
	int i;

	for (i = 0; i != graph->nodes; ++i)
		ll_free(&graph->neighbors[i]);

	free(graph->neighbors);
	free(graph);
}

int print_min_path(list_graph_t *graph, int src, int dest)
{
	if (!graph || !is_node_in_graph(src, graph->nodes) ||
		!is_node_in_graph(dest, graph->nodes))
		return 0;

	int *distances = malloc(graph->nodes * sizeof(int));
	int *predecessors = malloc(graph->nodes * sizeof(int));
	DIE(!distances || !predecessors, "malloc failed");
	for (int i = 0; i < graph->nodes; ++i)
	{
		distances[i] = 1000000;
		predecessors[i] = -1;
	}

	queue_t *priority_queue = q_create(sizeof(int), graph->nodes);
	DIE(!priority_queue, "q_create failed");
	q_enqueue(priority_queue, &src);
	distances[src] = 0;

	while (!q_is_empty(priority_queue))
	{
		int *current_node_ptr = q_front(priority_queue);
		int current_node = *current_node_ptr;
		q_dequeue(priority_queue);
		linked_list_t *neighbors = lg_get_neighbours(graph, current_node);
		if (!neighbors)
			continue;

		ll_node_t *neighbor_node = neighbors->head;
		while (neighbor_node)
		{
			int neighbor = *((int *)neighbor_node->data);
			if (distances[neighbor] > distances[current_node] + 1)
			{
				distances[neighbor] = distances[current_node] + 1;
				predecessors[neighbor] = current_node;
				q_enqueue(priority_queue, &neighbor);
			}

			neighbor_node = neighbor_node->next;
		}
	}
	q_free(priority_queue);
	if (predecessors[dest] == -1)
	{
		free(distances);
		free(predecessors);
		return 0;
	}
	linked_list_t *path = ll_create(sizeof(int));
	int current = dest;
	while (current != src)
	{
		ll_add_nth_node(path, 0, &current);
		current = predecessors[current];
	}
	ll_add_nth_node(path, 0, &src);
	free(distances);
	free(predecessors);
	int x = path->size - 1;
	ll_free(&path);
	return x;
}
