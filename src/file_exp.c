#include "file_exp.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

#include <dirent.h>
#include <unistd.h>

#include "conf.h"
#include "draw.h"
#include "keybd.h"
#include "util.h"

#define BIND_RET K_RET
#define BIND_QUIT K_CTL('g')
#define BIND_NAV_DOWN K_CTL('n')
#define BIND_NAV_UP K_CTL('p')

VEC_DEF_PROTO_STATIC(struct dir_node *, p_dir_node)

enum dir_node_type
{
	DNT_FILE = 0,
	DNT_DIR,
};

struct dir_node
{
	struct dir_node const *parent;
	struct vec_p_dir_node children;
	char *name;
	unsigned char type;
};

struct bounds
{
	unsigned pr, pc;
	unsigned sr, sc;
};

static struct dir_node *dir_tree(char const *root_name, char const *root_dir);
static void dir_node_unfold(struct dir_node *node);
static void dir_node_fold(struct dir_node *node);
static void dir_node_path(char *out_path, struct dir_node const *node);
static void dir_node_destroy(struct dir_node *node);
static void draw_box(struct bounds const *bounds, struct dir_node const *root, size_t first, size_t sel);
static struct dir_node const *dir_node_next(struct dir_node const *node);
static size_t dir_node_depth(struct dir_node const *node);
static int dir_node_cmp(void const *vp_lhs, void const *vp_rhs);
static size_t dir_tree_size(struct dir_node const *root);
static struct dir_node *dir_tree_nth(struct dir_node *root, size_t n);

VEC_DEF_IMPL_STATIC(struct dir_node *, p_dir_node)

enum file_exp_rc
file_exp_open(char *out_path, size_t n, char const *dir)
{
	struct dir_node *root = dir_tree(dir, dir);
	if (!root)
		return FER_FAIL;
	
	struct win_size ws = draw_win_size();
	struct bounds bounds =
	{
		.pr = 0,
		.sc = MIN(ws.sc, CONF_FILE_EXP_SC),
		.sr = ws.sr,
	};
	bounds.pc = ws.sc - bounds.sc;
	
	size_t first = 0, sel = 0;
	for (;;)
	{
		draw_box(&bounds, root, first, sel);
		draw_refresh();
		
		wint_t k = keybd_await_key_nb();
		switch (k)
		{
		case WEOF:
		case BIND_QUIT:
			dir_node_destroy(root);
			return FER_QUIT;
		case BIND_NAV_DOWN:
			sel += sel < dir_tree_size(root) - 1;
			first += sel >= bounds.sr + first;
			break;
		case BIND_NAV_UP:
			sel -= sel > 0;
			first -= sel < first;
			break;
		case BIND_RET:
		{
			struct dir_node *sel_node = dir_tree_nth(root, sel);
			if (!sel_node)
			{
				dir_node_destroy(root);
				return FER_FAIL;
			}
			
			if (sel_node->type == DNT_FILE)
			{
				dir_node_path(out_path, sel_node);
				return FER_SUCCESS;
			}
			
			if (sel_node->children.size)
				dir_node_fold(sel_node);
			else
				dir_node_unfold(sel_node);
			
			break;
		}
		default:
			break;
		}
	}
}

static struct dir_node *
dir_tree(char const *root_name, char const *root_dir)
{
	DIR *dir_p = opendir(root_dir);
	if (!dir_p)
		return NULL;
	
	struct dir_node *root = malloc(sizeof(struct dir_node));
	*root = (struct dir_node)
	{
		.parent = NULL,
		.children = vec_p_dir_node_create(),
		.name = strdup(root_name),
		.type = DNT_DIR,
	};
	
	struct dirent *dir_ent;
	while (dir_ent = readdir(dir_p))
	{
		if (dir_ent->d_type != DT_DIR && dir_ent->d_type != DT_REG
		    || !strcmp(".", dir_ent->d_name)
		    || !strcmp("..", dir_ent->d_name))
		{
			continue;
		}
		
		struct dir_node *child = malloc(sizeof(struct dir_node));
		*child = (struct dir_node)
		{
			.parent = root,
			.children = vec_p_dir_node_create(),
			.name = strdup(dir_ent->d_name),
			.type = dir_ent->d_type == DT_DIR ? DNT_DIR : DNT_FILE,
		};
		
		vec_p_dir_node_add(&root->children, &child);
	}
	
	qsort(root->children.data,
	      root->children.size,
	      sizeof(struct dir_node *),
	      dir_node_cmp);
	
	closedir(dir_p);
	
	return root;
}

static void
dir_node_unfold(struct dir_node *node)
{
	if (node->type != DNT_DIR)
		return;
	
	char path[PATH_MAX + 1];
	dir_node_path(path, node);
	
	struct dir_node *new = dir_tree(node->name, path);
	if (!new)
		return;
	
	vec_p_dir_node_destroy(&node->children);
	free(node->name);
	
	for (size_t i = 0; i < new->children.size; ++i)
		new->children.data[i]->parent = node;
	
	new->parent = node->parent;
	*node = *new;
	
	free(new);
}

static void
dir_node_fold(struct dir_node *node)
{
	if (node->type != DNT_DIR)
		return;
	
	for (size_t i = 0; i < node->children.size; ++i)
		dir_node_destroy(node->children.data[i]);
	node->children.size = 0;
}

static void
dir_node_path(char *out_path, struct dir_node const *node)
{
	struct vec_str names = vec_str_create();
	for (struct dir_node const *n = node->parent; n; n = n->parent)
	{
		char *name = strdup(n->name);
		vec_str_add(&names, &name);
	}
	
	out_path[0] = 0;
	for (size_t i = names.size; i > 0; --i)
	{
		strcat(out_path, names.data[i - 1]);
		strcat(out_path, "/");
	}
	strcat(out_path, node->name);
	
	for (size_t i = 0; i < names.size; ++i)
		free(names.data[i]);
	vec_str_destroy(&names);
}

static void
dir_node_destroy(struct dir_node *node)
{
	for (size_t i = 0; i < node->children.size; ++i)
		dir_node_destroy(node->children.data[i]);
	
	vec_p_dir_node_destroy(&node->children);
	free(node->name);
	free(node);
}

static void
draw_box(struct bounds const *bounds,
         struct dir_node const *root,
         size_t first,
         size_t sel)
{
	// fill explorer.
	draw_fill(bounds->pr,
	          bounds->pc,
	          bounds->sr,
	          bounds->sc,
	          L' ',
	          CONF_A_GHIGH_BG,
	          CONF_A_GHIGH_FG);
	
	// draw files and directories.
	struct dir_node const *node = root;
	for (size_t i = 0; node; ++i, node = dir_node_next(node))
	{
		if (i < first)
			continue;
		
		if (i >= bounds->sr + first)
			break;
		
		unsigned depth = CONF_FILE_EXP_NEST_DEPTH * dir_node_depth(node);
		size_t name_len = strlen(node->name);
		for (unsigned j = 0; j < name_len && j + depth < bounds->sc; ++j)
		{
			draw_put_wch(bounds->pr + i - first,
			             bounds->pc + j + depth,
			             node->name[j]);
		}
		
		if (i == sel)
		{
			draw_put_attr(bounds->pr + i - first,
			              bounds->pc,
			              CONF_A_GHIGH_FG,
			              CONF_A_GHIGH_BG,
			              bounds->sc);
		}
	}
}

static struct dir_node const *
dir_node_next(struct dir_node const *node)
{
	if (node->children.size)
		return node->children.data[0];
	
	if (!node->parent)
		return NULL;
	
	size_t node_ind = 0;
	while (node->parent->children.data[node_ind] != node)
		++node_ind;
	
	if (node_ind < node->parent->children.size - 1)
		return node->parent->children.data[node_ind + 1];
	
	// hack to allow upwards-recursive call to `dir_node_next()` without
	// having to implement a bunch of checks, by temporarily tricking the
	// parent node into thinking it has no children.
	// casting const away isn't an issue since nothing is actually modified in
	// the end.
	struct dir_node *parent_mut = (struct dir_node *)node->parent;
	
	size_t sv_nchildren = node->parent->children.size;
	parent_mut->children.size = 0;
	struct dir_node const *next_parent = dir_node_next(node->parent);
	parent_mut->children.size = sv_nchildren;
	
	return next_parent;
}

static size_t
dir_node_depth(struct dir_node const *node)
{
	if (!node)
		return 0;
	
	size_t depth = 0;
	while (node = node->parent)
		++depth;
	
	return depth;
}

static int
dir_node_cmp(void const *vp_lhs, void const *vp_rhs)
{
	struct dir_node const *const *lhs = vp_lhs, *const *rhs = vp_rhs;
	return strcmp((*lhs)->name, (*rhs)->name);
}

static size_t
dir_tree_size(struct dir_node const *root)
{
	size_t nnodes = 0;
	for (struct dir_node const *node = root; node; node = dir_node_next(node))
		++nnodes;
	return nnodes;
}

static struct dir_node *
dir_tree_nth(struct dir_node *root, size_t n)
{
	struct dir_node *node = root;
	for (size_t i = 0; i < n; ++i)
	{
		// casting away const is fine since nothing here is actually
		// modified, and the caller doesn't care how the nth node is
		// obtained as long as it's correct.
		node = (struct dir_node *)dir_node_next(node);
		if (!node)
			return NULL;
	}
	return node;
}
