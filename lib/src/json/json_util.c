#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include "json_util.h"

#define JSON_BUF_START_LEN 8192

jbuf_t jbuf_new(void)
{
	jbuf_t jb = malloc(sizeof(*jb) + JSON_BUF_START_LEN);
	if (jb) {
		jb->buf_len = JSON_BUF_START_LEN;
		jb->cursor = 0;
	}
	return jb;
}

void jbuf_free(jbuf_t jb)
{
	free(jb);
}

jbuf_t jbuf_append_va(jbuf_t jb, const char *fmt, va_list ap)
{
	int cnt, space;
 retry:
	space = jb->buf_len - jb->cursor;
	cnt = vsnprintf(&jb->buf[jb->cursor], space, fmt, ap);
	if (cnt > space) {
		space = jb->buf_len + JSON_BUF_START_LEN;
		jb = realloc(jb, space);
		if (jb) {
			jb->buf_len = space;
			goto retry;
		} else {
			return NULL;
		}
	}
	jb->cursor += cnt;
	return jb;
}

jbuf_t jbuf_append_str(jbuf_t jb, const char *fmt, ...)
{
	int cnt, space;
	va_list ap;
	va_start(ap, fmt);
	jb = jbuf_append_va(jb, fmt, ap);
	va_end(ap);
	return jb;
}

jbuf_t jbuf_append_attr(jbuf_t jb, const char *name, const char *fmt, ...)
{
	int cnt, space;
	va_list ap;
	va_start(ap, fmt);
	jb = jbuf_append_str(jb, "\"%s\":", name);
	if (jb)
		jb = jbuf_append_va(jb, fmt, ap);
	va_end(ap);
	return jb;
}

json_entity_t json_attr_first(json_entity_t d)
{
	hent_t ent;
	assert(d->type == JSON_DICT_VALUE);
	ent = htbl_first(d->value.dict_->attr_table);
	if (!ent)
		return NULL;
	json_attr_t a = container_of(ent, struct json_attr_s, attr_ent);
	return &a->base;
}

json_entity_t json_attr_next(json_entity_t a)
{
	hent_t ent;
	json_attr_t next;
	assert(a->type == JSON_ATTR_VALUE);
	ent = htbl_next(&a->value.attr_->attr_ent);
	if (ent) {
		next = container_of(ent, struct json_attr_s, attr_ent);
		return &next->base;
	}
	return NULL;
}

json_entity_t json_attr_find(json_entity_t d, char *name)
{
	hent_t ent;
	json_attr_t a;
	assert (d->type == JSON_DICT_VALUE);
	ent = htbl_find(d->value.dict_->attr_table, name, strlen(name));
	if (ent) {
		a = container_of(ent, struct json_attr_s, attr_ent);
		return &a->base;
	}
	return NULL;
}

int json_attr_count(json_entity_t d)
{
	assert(d->type == JSON_DICT_VALUE);
	json_dict_t dict = (json_dict_t)d;
	return dict->attr_table->entry_count;
}

const char *json_attr_find_str(json_entity_t d, char *name)
{
	json_entity_t attr;
	attr = json_attr_find(d, name);
	if (!attr)
		return NULL;
	return json_attr_value_str(attr);
}

int attr_cmp(const void *a, const void *b, size_t key_len)
{
	return strncmp(a, b, key_len);
}

#define JSON_HTBL_DEPTH	23

static json_entity_t json_dict_new(void)
{
	json_dict_t d = malloc(sizeof *d);
	if (d) {
		d->base.type = JSON_DICT_VALUE;
		d->base.value.dict_ = d;
		d->attr_table = htbl_alloc(attr_cmp, JSON_HTBL_DEPTH);
		if (!d->attr_table) {
			free(d);
			return NULL;
		}
		return &d->base;
	}
	return NULL;
}

static json_entity_t json_str_new(const char *s)
{
	json_str_t str = malloc(sizeof *str);
	if (str) {
		str->base.type = JSON_STRING_VALUE;
		str->base.value.str_ = str;
		str->str = strdup(s);
		if (!str->str) {
			free(str);
			return NULL;
		}
		str->str_len = strlen(s);
		return &str->base;
	}
	return NULL;
}

static json_entity_t json_list_new(void)
{
	json_list_t a = malloc(sizeof *a);
	if (a) {
		a->base.type = JSON_LIST_VALUE;
		a->base.value.list_ = a;
		a->item_count = 0;
		TAILQ_INIT(&a->item_list);
		return &a->base;
	}
	return NULL;
}

void json_item_add(json_entity_t a, json_entity_t e)
{
	assert(a->type == JSON_LIST_VALUE);
	a->value.list_->item_count++;
	TAILQ_INSERT_TAIL(&a->value.list_->item_list, e, item_entry);
}

json_entity_t json_item_first(json_entity_t a)
{
	json_entity_t i;
	assert(a->type == JSON_LIST_VALUE);
	i = TAILQ_FIRST(&a->value.list_->item_list);
	return i;
}

json_entity_t json_item_next(json_entity_t a)
{
	return TAILQ_NEXT(a, item_entry);
}

static json_entity_t json_attr_new(json_entity_t name, json_entity_t value)
{
	json_attr_t a = malloc(sizeof *a);
	if (a) {
		a->base.type = JSON_ATTR_VALUE;
		a->base.value.attr_ = a;
		a->name = name;
		a->value = value;
		return &a->base;
	}
	return NULL;
}

json_entity_t json_entity_new(enum json_value_e type, ...)
{
	uint64_t i;
	double d;
	char *s;
	va_list ap;
	json_entity_t e, name, value;

	va_start(ap, type);
	switch (type) {
	case JSON_INT_VALUE:
		e = malloc(sizeof *e);
		if (!e)
			goto out;
		e->type = type;
		i = va_arg(ap, uint64_t);
		e->value.int_ = i;
		break;
	case JSON_BOOL_VALUE:
		e = malloc(sizeof *e);
		if (!e)
			goto out;
		e->type = type;
		i = va_arg(ap, int);
		e->value.bool_ = i;
		break;
	case JSON_FLOAT_VALUE:
		e = malloc(sizeof *e);
		if (!e)
			goto out;
		e->type = type;
		d = va_arg(ap, double);
		e->value.double_ = d;
		break;
	case JSON_STRING_VALUE:
		s = va_arg(ap, char *);
		e = json_str_new(s);
		break;
	case JSON_ATTR_VALUE:
		name = va_arg(ap, json_entity_t);
		assert(JSON_STRING_VALUE == name->type);
		value = va_arg(ap, json_entity_t);
		e = json_attr_new(name, value);
		break;
	case JSON_LIST_VALUE:
		e = json_list_new();
		break;
	case JSON_DICT_VALUE:
		e = json_dict_new();
		break;
	case JSON_NULL_VALUE:
		break;
	default:
		assert(0 == "Invalid entity type");
	}
 out:
	va_end(ap);
	return e;
 err:
	free(e);
	goto out;
}

static jbuf_t __entity_dump(jbuf_t jb, json_entity_t e);
static jbuf_t __list_dump(jbuf_t jb, json_entity_t e)
{
	json_entity_t i;
	int count = 0;
	jb = jbuf_append_str(jb, "[");
	for (i = json_item_first(e); i; i = json_item_next(i)) {
		if (count)
			jb = jbuf_append_str(jb, ",");
		__entity_dump(jb, i);
		count++;
	}
	jb = jbuf_append_str(jb, "]");
	return jb;
}

static jbuf_t __dict_dump(jbuf_t jb, json_entity_t e)
{
	json_entity_t a;
	hent_t ent;
	int count = 0;
	jb = jbuf_append_str(jb, "{");
	for (a = json_attr_first(e); a; a = json_attr_next(a)) {
		if (count)
			jb = jbuf_append_str(jb, ",");
		jb = jbuf_append_str(jb, "\"%s\":", a->value.attr_->name->value.str_->str);
		__entity_dump(jb, a->value.attr_->value);
		count++;
	}
	jb = jbuf_append_str(jb, "}");
	return jb;
}

static jbuf_t __entity_dump(jbuf_t jb, json_entity_t e)
{
	switch (e->type) {
	case JSON_INT_VALUE:
		jb = jbuf_append_str(jb, "%ld", e->value.int_);
		break;
	case JSON_BOOL_VALUE:
		if (e->value.bool_)
			jb = jbuf_append_str(jb, "true");
		else
			jb = jbuf_append_str(jb, "false");
		break;
	case JSON_FLOAT_VALUE:
		jb = jbuf_append_str(jb, "%f", e->value.double_);
		break;
	case JSON_STRING_VALUE:
		jb = jbuf_append_str(jb, "\"%s\"", e->value.str_->str);
		break;
	case JSON_LIST_VALUE:
		__list_dump(jb, e);
		break;
	case JSON_DICT_VALUE:
		__dict_dump(jb, e);
		break;
	case JSON_NULL_VALUE:
		jb = jbuf_append_str(jb, "null");
		break;
	default:
		fprintf(stderr, "%d is an invalid entity type", e->type);
	}
	return jb;
}

jbuf_t json_entity_dump(jbuf_t jb, json_entity_t e)
{
	if (!jb) {
		jb = jbuf_new();
		if (!jb)
			return NULL;
	}
	jb = __entity_dump(jb, e);
	return jb;
}

json_entity_t json_entity_copy(json_entity_t e)
{
	json_entity_t new, n, v;
	enum json_value_e type = json_entity_type(e);

	switch (type) {
	case JSON_INT_VALUE:
	case JSON_BOOL_VALUE:
		new = json_entity_new(type, e->value);
		break;
	case JSON_FLOAT_VALUE:
		new = json_entity_new(type, e->value.double_);
		break;
	case JSON_STRING_VALUE:
		new = json_entity_new(type, json_value_str(e)->str);
		break;
	case JSON_ATTR_VALUE:
		n = json_entity_new(JSON_STRING_VALUE, json_attr_name(e)->str);
		if (!n)
			return NULL;
		v = json_entity_copy(json_attr_value(e));
		if (!v) {
			json_entity_free(n);
			return NULL;
		}
		new = json_entity_new(type, n, v);
		if (!new) {
			json_entity_free(n);
			json_entity_free(v);
			return NULL;
		}
		break;
	case JSON_LIST_VALUE:
		new = json_entity_new(type);
		if (!new)
			return NULL;
		for (n = json_item_first(e); n; n = json_item_next(n)) {
			v = json_entity_copy(n);
			if (!v) {
				json_entity_free(new);
				return NULL;
			}
			json_item_add(new, v);
		}
		break;
	case JSON_DICT_VALUE:
		new = json_entity_new(type);
		if (!new)
			return NULL;
		for (n = json_attr_first(e); n; n = json_attr_next(n)) {
			v = json_entity_copy(n);
			if (!v) {
				json_entity_free(new);
				return NULL;
			}
			json_attr_add(new, v);
		}
		break;
	default:
		assert(0 == "Invalid entity type");
		break;
	}
	return new;
}

static void json_attr_free(json_attr_t a);
static inline __attr_rem(json_entity_t d, json_entity_t a)
{
	htbl_del(d->value.dict_->attr_table, &a->value.attr_->attr_ent);
	json_attr_free(a->value.attr_);
}

void json_attr_add(json_entity_t d, json_entity_t a)
{
	json_entity_t a_;
	assert(d->type == JSON_DICT_VALUE);
	assert(a->type == JSON_ATTR_VALUE);
	json_str_t s = a->value.attr_->name->value.str_;
	hent_init(&a->value.attr_->attr_ent, s->str, s->str_len);
	a_ = json_attr_find(d, s->str);
	if (a_) {
		__attr_rem(d, a_);
	}
	htbl_ins(d->value.dict_->attr_table, &a->value.attr_->attr_ent);
}

static void json_dict_free(json_dict_t d);
static void json_list_free(json_list_t l);
int json_attr_mod(json_entity_t d, char *name, ...)
{
	int i;
	double f;
	char *s;
	va_list ap;
	json_entity_t a, av;
	json_entity_t l, dd;

	assert(d->type == JSON_DICT_VALUE);
	a = json_attr_find(d, name);
	if (!a)
		return ENOENT;
	av = json_attr_value(a);
	va_start(ap, name);
	switch (av->type) {
	case JSON_INT_VALUE:
		i = va_arg(ap, uint64_t);
		av->value.int_ = i;
		break;
	case JSON_BOOL_VALUE:
		i = va_arg(ap, int);
		av->value.bool_ = i;
		break;
	case JSON_FLOAT_VALUE:
		f = va_arg(ap, double);
		av->value.double_ = f;
		break;
	case JSON_STRING_VALUE:
		s = va_arg(ap, char *);
		json_entity_free(av);
		a->value.attr_->value = json_str_new(s);
		break;
	case JSON_ATTR_VALUE:
		json_entity_free(av);
		a->value.attr_->value = va_arg(ap, json_entity_t);
		break;
	case JSON_LIST_VALUE:
		json_entity_free(av);
		a->value.attr_->value = va_arg(ap, json_entity_t);
		break;
	case JSON_DICT_VALUE:
		json_entity_free(av);
		a->value.attr_->value = va_arg(ap, json_entity_t);
		break;
	case JSON_NULL_VALUE:
		break;
	default:
		/* The attribute value must not be of ATTR type */
		assert(0 == "Invalid entity type");
	}
	va_end(ap);
	return 0;
}

static void json_attr_free(json_attr_t a);
int json_attr_rem(json_entity_t d, char *name)
{
	json_entity_t a;
	assert(d->type == JSON_DICT_VALUE);
	a = json_attr_find(d, name);
	if (!a)
		return ENOENT;
	__attr_rem(d, a);
	return 0;
}

static void json_list_free(json_list_t a)
{
	json_entity_t i;
	assert(a->base.type == JSON_LIST_VALUE);
	while (!TAILQ_EMPTY(&a->item_list)) {
		i = TAILQ_FIRST(&a->item_list);
		TAILQ_REMOVE(&a->item_list, i, item_entry);
		json_entity_free(i);
	}
	free(a);
}

static void json_str_free(json_str_t s)
{
	assert(s->base.type == JSON_STRING_VALUE);
	free(s->str);
	free(s);
}

static void json_attr_free(json_attr_t a)
{
	assert(a->base.type == JSON_ATTR_VALUE);
	json_entity_free(a->name);
	json_entity_free(a->value);
	free(a);
}

static void json_dict_free(json_dict_t d)
{
	json_attr_t i;
	hent_t ent;
	htbl_t t;

	t = d->attr_table;
	while (!htbl_empty(t)) {
		ent = htbl_first(t);
		htbl_del(t, ent);
		i = container_of(ent, struct json_attr_s, attr_ent);
		json_attr_free(i);
	}
	htbl_free(t);
	free(d);
}

void json_entity_free(json_entity_t e)
{
	if (!e)
		return;
	switch (e->type) {
	case JSON_INT_VALUE:
		free(e);
		break;
	case JSON_BOOL_VALUE:
		free(e);
		break;
	case JSON_FLOAT_VALUE:
		free(e);
		break;
	case JSON_STRING_VALUE:
		json_str_free(e->value.str_);
		break;
	case JSON_ATTR_VALUE:
		json_attr_free(e->value.attr_);
		break;
	case JSON_LIST_VALUE:
		json_list_free(e->value.list_);
		break;
	case JSON_DICT_VALUE:
		json_dict_free(e->value.dict_);
		break;
	case JSON_NULL_VALUE:
		free(e);
		break;
	default:
		/* Leak if we're passed garbage */
		return;
	}
}

enum json_value_e json_entity_type(json_entity_t e)
{
	return e->type;
}

int64_t json_value_int(json_entity_t value)
{
	assert(value->type == JSON_INT_VALUE);
	return value->value.int_;
}

int json_value_bool(json_entity_t value)
{
	assert(value->type == JSON_BOOL_VALUE);
	return value->value.bool_;
}

double json_value_float(json_entity_t value)
{
	assert(value->type == JSON_FLOAT_VALUE);
	return value->value.double_;
}

json_str_t json_value_str(json_entity_t value)
{
	assert(value->type == JSON_STRING_VALUE);
	return value->value.str_;
}

json_attr_t json_value_attr(json_entity_t value)
{
	assert(value->type == JSON_ATTR_VALUE);
	return value->value.attr_;
}

json_list_t json_value_list(json_entity_t value)
{
	assert(value->type == JSON_LIST_VALUE);
	return value->value.list_;
}

json_dict_t json_value_dict(json_entity_t value)
{
	assert(value->type == JSON_DICT_VALUE);
	return value->value.dict_;
}

json_str_t json_attr_name(json_entity_t attr)
{
	assert(attr->type == JSON_ATTR_VALUE);
	return attr->value.attr_->name->value.str_;
}

json_entity_t json_attr_value(json_entity_t attr)
{
	assert(attr->type == JSON_ATTR_VALUE);
	return attr->value.attr_->value;
}

const char *json_attr_value_str(json_entity_t attr)
{
	return json_value_str(json_attr_value(attr))->str;
}

int64_t json_attr_value_int(json_entity_t attr)
{
	return json_value_int(json_attr_value(attr));
}

int json_attr_value_bool(json_entity_t attr)
{
	return json_value_bool(json_attr_value(attr));
}

double json_attr_value_float(json_entity_t attr)
{
	return json_value_float(json_attr_value(attr));
}

size_t json_list_len(json_entity_t list)
{
	assert(list->type == JSON_LIST_VALUE);
	return list->value.list_->item_count;
}