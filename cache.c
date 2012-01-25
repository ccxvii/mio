#include "mio.h"

// Use an AA-tree to quickly look up resources.

struct cache
{
	char *key;
	void *value;
	struct cache *left, *right;
	int level;
};

static struct cache sentinel = { "", NULL, &sentinel, &sentinel, 0 };

static struct cache *make_node(char *key, void *value)
{
	struct cache *node = malloc(sizeof(struct cache));
	node->key = strdup(key);
	node->value = value;
	node->left = node->right = &sentinel;
	node->level = 1;
	return node;
}

void *lookup(struct cache *node, char *key)
{
	if (node && node != &sentinel) {
		int c = strcmp(key, node->key);
		if (c == 0)
			return node->value;
		else if (c < 0)
			return lookup(node->left, key);
		else
			return lookup(node->right, key);
	}
	return NULL;
}

static struct cache *skew(struct cache *node)
{
	if (node->level != 0) {
		if (node->left->level == node->level) {
			struct cache *save = node;
			node = node->left;
			save->left = node->right;
			node->right = save;
		}
		node->right = skew(node->right);
	}
	return node;
}

static struct cache *split(struct cache *node)
{
	if (node->level != 0 && node->right->right->level == node->level) {
		struct cache *save = node;
		node = node->right;
		save->right = node->left;
		node->left = save;
		node->level++;
		node->right = split(node->right);
	}
	return node;
}

struct cache *insert(struct cache *node, char *key, void *value)
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
	} else {
		return make_node(key, value);
	}
}

static void debug_cache_imp(struct cache *node, int level)
{
	int i;
	if (node->left != &sentinel)
		debug_cache_imp(node->left, level + 1);
	for (i = 0; i < level; i++)
		putchar(' ');
	printf("%s = %p (%d)\n", node->key, node->value, node->level);
	if (node->right != &sentinel)
		debug_cache_imp(node->right, level + 1);
}

void debug_cache(struct cache *root)
{
	printf("--- cache dump ---\n");
	if (root && root != &sentinel)
		debug_cache_imp(root, 0);
	printf("---\n");
}
