#include "mio.h"

/* Use an AA-tree to quickly look up resources. */

struct cache
{
	char *key;
	void *value;
	struct cache *left, *right;
	int level;
};

static struct cache sentinel = { "", NULL, &sentinel, &sentinel, 0 };

static struct cache *make_node(const char *key, void *value)
{
	struct cache *node = malloc(sizeof(struct cache));
	node->key = strdup(key);
	node->value = value;
	node->left = node->right = &sentinel;
	node->level = 1;
	return node;
}

void *lookup(struct cache *node, const char *key)
{
	if (node) {
		while (node != &sentinel) {
			int c = strcmp(key, node->key);
			if (c == 0)
				return node->value;
			else if (c < 0)
				node = node->left;
			else
				node = node->right;
		}
	}
	return NULL;
}

static struct cache *skew(struct cache *node)
{
	if (node->left->level == node->level) {
		struct cache *save = node;
		node = node->left;
		save->left = node->right;
		node->right = save;
	}
	return node;
}

static struct cache *split(struct cache *node)
{
	if (node->right->right->level == node->level) {
		struct cache *save = node;
		node = node->right;
		save->right = node->left;
		node->left = save;
		node->level++;
	}
	return node;
}

struct cache *insert(struct cache *node, const char *key, void *value)
{
	if (node && node != &sentinel) {
		int c = strcmp(key, node->key);
		if (c < 0)
			node->left = insert(node->left, key, value);
		else
			node->right = insert(node->right, key, value);
		node = skew(node);
		node = split(node);
		return node;
	}
	return make_node(key, value);
}

static void print_cache_imp(struct cache *node, int level)
{
	int i;
	if (node->left != &sentinel)
		print_cache_imp(node->left, level + 1);
	for (i = 0; i < level; i++)
		putchar(' ');
	printf("%s = %p (%d)\n", node->key, node->value, node->level);
	if (node->right != &sentinel)
		print_cache_imp(node->right, level + 1);
}

void print_cache(struct cache *root)
{
	printf("--- cache dump ---\n");
	if (root && root != &sentinel)
		print_cache_imp(root, 0);
	printf("---\n");
}
