#include "file_exp.h"

#include <limits.h>
#include <stddef.h>
#include <wchar.h>

#include <dirent.h>
#include <sys/ioctl.h>
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

enum dir_node_flag
{
	// directory flags.
	DNF_UNFOLDED = 0x1,
};

struct dir_node
{
	struct dir_node const *parent;
	struct vec_p_dir_node children;
	char *name;
	unsigned char type;
	unsigned char flags;
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

VEC_DEF_IMPL_STATIC(struct dir_node *, p_dir_node)

enum file_exp_rc
file_exp_open(char *out_path, size_t n, char const *dir)
{
	struct dir_node *root = dir_tree(".", dir);
	if (!root)
		return FER_FAIL;
	
	struct winsize ws;
	ioctl(0, TIOCGWINSZ, &ws);
	
	struct bounds bounds =
	{
		.pr = 0,
		.sc = MIN(ws.ws_col, CONF_FILE_EXP_SC),
		.sr = ws.ws_row,
	};
	bounds.pc = ws.ws_col - bounds.sc;
	
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
			// TODO: implement.
			break;
		case BIND_NAV_UP:
			// TODO: implement.
			break;
		case BIND_RET:
			// TODO: implement.
			break;
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
		.flags = 0,
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
			.flags = 0,
		};
		
		vec_p_dir_node_add(&root->children, &child);
	}
	
	closedir(dir_p);
	
	return root;
}

static void
dir_node_unfold(struct dir_node *node)
{
	if (node->type != DNT_DIR)
		return;
	
	if (node->flags & DNF_UNFOLDED)
		return;
	
	char path[PATH_MAX + 1];
	dir_node_path(path, node);
	
	struct dir_node *new = dir_tree(node->name, path);
	if (!new)
		return;
	
	new->parent = node->parent;
	*node = *new;
}

static void
dir_node_fold(struct dir_node *node)
{
	if (node->type != DNT_DIR)
		return;
	
	if (!(node->flags & DNF_UNFOLDED))
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
	// TODO: implement.
}
