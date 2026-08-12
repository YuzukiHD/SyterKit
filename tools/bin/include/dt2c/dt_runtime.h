/* SPDX-License-Identifier: MIT */
#ifndef DT2C_DT_RUNTIME_H
#define DT2C_DT_RUNTIME_H

#define __DT2C_INLINE static inline __attribute__((always_inline))

__DT2C_INLINE int __dt2c_fdt_valid(const void *fdt)
{
	return fdt == DT2C_FDT_COMPILED_TREE;
}

__DT2C_INLINE size_t __dt2c_strnlen(const char *value, size_t maximum)
{
	const char *end = (const char *)__builtin_memchr(value, 0, maximum);

	return end ? (size_t)(end - value) : maximum;
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
__DT2C_INLINE int __dt2c_namelen_equal(const char *left, int left_length,
				      const char *right)
{
	return left_length >= 0 &&
	       (size_t)left_length == __builtin_strlen(right) &&
	       __builtin_memcmp(left, right, (size_t)left_length) == 0;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

__DT2C_INLINE int __dt2c_node_name_equal(const char *node_name,
					const char *name, int length)
{
	size_t node_length = __builtin_strlen(node_name);

	if (length < 0 || node_length < (size_t)length ||
	    __builtin_memcmp(node_name, name, (size_t)length) != 0)
		return 0;
	return node_name[length] == '\0' ||
	       (!__builtin_memchr(name, '@', (size_t)length) &&
		node_name[length] == '@');
}

__DT2C_INLINE int __dt2c_node_valid(int offset)
{
	switch (offset) {
#define __DT2C_NODE_VALID(_index, _offset, ...) case _offset: return 1;
	__DT2C_FOREACH_NODE(__DT2C_NODE_VALID)
#undef __DT2C_NODE_VALID
	default:
		return 0;
	}
}

__DT2C_INLINE int dt2c_fdt_node_is_available(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_NODE_AVAILABLE(_offset, _available)                         \
	if (nodeoffset == (_offset))                                          \
		return (_available);
	__DT2C_FOREACH_NODE_AVAILABILITY(__DT2C_NODE_AVAILABLE)
#undef __DT2C_NODE_AVAILABLE
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE const void *dt2c_fdt_offset_ptr(const void *fdt, int offset,
					unsigned int checklen)
{
	unsigned int structure_offset;

	if (!__dt2c_fdt_valid(fdt) || offset < 0)
		return NULL;
	structure_offset = (unsigned int)offset;
	if (structure_offset > __DT2C_FDT_STRUCT_SIZE ||
	    checklen > __DT2C_FDT_STRUCT_SIZE - structure_offset)
		return NULL;
	return __dt2c_fdt_structure_storage + structure_offset;
}

__DT2C_INLINE uint32_t dt2c_fdt_next_tag(const void *fdt, int startoffset,
				   int *nextoffset)
{
	*nextoffset = -DT2C_FDT_ERR_TRUNCATED;
	if (!__dt2c_fdt_valid(fdt) || startoffset < 0 ||
	    (unsigned int)startoffset > __DT2C_FDT_STRUCT_SIZE - DT2C_FDT_TAGSIZE)
		return DT2C_FDT_END;
#define __DT2C_NEXT_TAG(_offset, _tag, _next, _storage, _length)            \
	if (startoffset == (_offset)) {                                       \
		*nextoffset = (_next);                                         \
		return (_tag);                                                \
	}
	__DT2C_FOREACH_OFFSET(__DT2C_NEXT_TAG)
#undef __DT2C_NEXT_TAG
	*nextoffset = -DT2C_FDT_ERR_BADSTRUCTURE;
	return DT2C_FDT_END;
}

__DT2C_INLINE int dt2c_fdt_check_header(const void *fdt)
{
	return __dt2c_fdt_valid(fdt) ? 0 : -DT2C_FDT_ERR_BADMAGIC;
}

__DT2C_INLINE int dt2c_fdt_check_full(const void *fdt, size_t bufsize)
{
	int error = dt2c_fdt_check_header(fdt);

	if (error < 0)
		return error;
	return bufsize < __DT2C_FDT_TOTAL_SIZE ? -DT2C_FDT_ERR_TRUNCATED : 0;
}

__DT2C_INLINE const char *dt2c_fdt_get_string(const void *fdt, int stroffset,
					int *lenp)
{
	const char *result;

	if (!__dt2c_fdt_valid(fdt)) {
		if (lenp)
			*lenp = -DT2C_FDT_ERR_BADMAGIC;
		return NULL;
	}
	if (stroffset >= 0 && (unsigned int)stroffset < __DT2C_FDT_STRINGS_SIZE) {
		result = (const char *)__dt2c_fdt_strings_storage + stroffset;
		if (lenp)
			*lenp = (int)__builtin_strlen(result);
		return result;
	}
	if (lenp)
		*lenp = -DT2C_FDT_ERR_BADOFFSET;
	return NULL;
}

__DT2C_INLINE const char *dt2c_fdt_string(const void *fdt, int stroffset)
{
	return dt2c_fdt_get_string(fdt, stroffset, NULL);
}

__DT2C_INLINE const struct dt2c_fdt_property *
dt2c_fdt_get_property_by_offset(const void *fdt, int offset, int *lenp)
{
	if (!__dt2c_fdt_valid(fdt)) {
		if (lenp)
			*lenp = -DT2C_FDT_ERR_BADMAGIC;
		return NULL;
	}
#define __DT2C_PROPERTY_BY_OFFSET(_index, _offset, _node, _name_offset,     \
				   _name, _storage, _next)                 \
	if (offset == (_offset)) {                                           \
		const struct dt2c_fdt_property *property =                         \
			(const struct dt2c_fdt_property *)&(_storage);             \
		if (lenp)                                                    \
			*lenp = (int)dt2c_fdt32_to_cpu(property->len);            \
		return property;                                            \
	}
	__DT2C_FOREACH_PROPERTY(__DT2C_PROPERTY_BY_OFFSET)
#undef __DT2C_PROPERTY_BY_OFFSET
	if (lenp)
		*lenp = -DT2C_FDT_ERR_BADOFFSET;
	return NULL;
}

__DT2C_INLINE int dt2c_fdt_first_property_offset(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_FIRST_PROPERTY(_index, _offset, _parent, _depth,            \
			       _first_child, _next_sibling, _first_property, \
			       _property_count, ...)                         \
	if (nodeoffset == (_offset))                                         \
		return (_property_count) ?                                   \
			       (_first_property) :                           \
			       -DT2C_FDT_ERR_NOTFOUND;
	__DT2C_FOREACH_NODE(__DT2C_FIRST_PROPERTY)
#undef __DT2C_FIRST_PROPERTY
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE int dt2c_fdt_next_property_offset(const void *fdt, int offset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_NEXT_PROPERTY(_index, _offset, _node, _name_offset, _name,  \
			      _storage, _next)                              \
	if (offset == (_offset))                                             \
		return (_next);
	__DT2C_FOREACH_PROPERTY(__DT2C_NEXT_PROPERTY)
#undef __DT2C_NEXT_PROPERTY
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE const struct dt2c_fdt_property *
dt2c_fdt_get_property_namelen(const void *fdt, int nodeoffset, const char *name,
			 int namelen, int *lenp)
{
	if (!__dt2c_fdt_valid(fdt)) {
		if (lenp)
			*lenp = -DT2C_FDT_ERR_BADMAGIC;
		return NULL;
	}
	if (!__dt2c_node_valid(nodeoffset)) {
		if (lenp)
			*lenp = -DT2C_FDT_ERR_BADOFFSET;
		return NULL;
	}
#define __DT2C_PROPERTY_BY_NAME(_index, _offset, _node, _name_offset,      \
				 _name, _storage, _next)                 \
	if (nodeoffset == (_node) &&                                         \
	    __dt2c_namelen_equal(name, namelen, (_name))) {                  \
		const struct dt2c_fdt_property *property =                         \
			(const struct dt2c_fdt_property *)&(_storage);             \
		if (lenp)                                                    \
			*lenp = (int)dt2c_fdt32_to_cpu(property->len);            \
		return property;                                            \
	}
	__DT2C_FOREACH_PROPERTY(__DT2C_PROPERTY_BY_NAME)
#undef __DT2C_PROPERTY_BY_NAME
	if (lenp)
		*lenp = -DT2C_FDT_ERR_NOTFOUND;
	return NULL;
}

__DT2C_INLINE const struct dt2c_fdt_property *
dt2c_fdt_get_property(const void *fdt, int nodeoffset, const char *name, int *lenp)
{
	return dt2c_fdt_get_property_namelen(fdt, nodeoffset, name,
					(int)__builtin_strlen(name), lenp);
}

__DT2C_INLINE const void *dt2c_fdt_getprop_namelen(const void *fdt, int nodeoffset,
					     const char *name, int namelen,
					     int *lenp)
{
	const struct dt2c_fdt_property *property =
		dt2c_fdt_get_property_namelen(fdt, nodeoffset, name, namelen, lenp);

	return property ? property->data : NULL;
}

__DT2C_INLINE const void *dt2c_fdt_getprop(const void *fdt, int nodeoffset,
				     const char *name, int *lenp)
{
	return dt2c_fdt_getprop_namelen(fdt, nodeoffset, name,
				   (int)__builtin_strlen(name), lenp);
}

__DT2C_INLINE const void *dt2c_fdt_getprop_by_offset(const void *fdt, int offset,
					       const char **namep, int *lenp)
{
	const struct dt2c_fdt_property *property =
		dt2c_fdt_get_property_by_offset(fdt, offset, lenp);

	if (!property)
		return NULL;
	if (namep)
		*namep = dt2c_fdt_string(fdt,
				    (int)dt2c_fdt32_to_cpu(property->nameoff));
	return property->data;
}

__DT2C_INLINE const char *dt2c_fdt_get_name(const void *fdt, int nodeoffset,
				      int *lenp)
{
	if (!__dt2c_fdt_valid(fdt)) {
		if (lenp)
			*lenp = -DT2C_FDT_ERR_BADMAGIC;
		return NULL;
	}
#define __DT2C_NODE_NAME(_index, _offset, _parent, _depth, _first_child,   \
			  _next_sibling, _first_property, _property_count, \
			  _next_node, _next_delta, _name, _path, _phandle) \
	if (nodeoffset == (_offset)) {                                       \
		if (lenp)                                                    \
			*lenp = (int)__builtin_strlen(_name);                \
		return (_name);                                              \
	}
	__DT2C_FOREACH_NODE(__DT2C_NODE_NAME)
#undef __DT2C_NODE_NAME
	if (lenp)
		*lenp = -DT2C_FDT_ERR_BADOFFSET;
	return NULL;
}

__DT2C_INLINE int dt2c_fdt_next_node(const void *fdt, int nodeoffset, int *depth)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (nodeoffset < 0) {
		if (depth)
			++*depth;
		return 0;
	}
#define __DT2C_NEXT_NODE(_index, _offset, _parent, _depth, _first_child,   \
			  _next_sibling, _first_property, _property_count, \
			  _next_node, _next_delta, _name, _path, _phandle) \
	if (nodeoffset == (_offset)) {                                       \
		if (depth) {                                                 \
			*depth += (_next_delta);                             \
			if ((_next_node) == -DT2C_FDT_ERR_NOTFOUND)               \
				return __DT2C_FDT_END_OFFSET;                 \
		}                                                            \
		return (_next_node);                                         \
	}
	__DT2C_FOREACH_NODE(__DT2C_NEXT_NODE)
#undef __DT2C_NEXT_NODE
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE int dt2c_fdt_first_subnode(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_FIRST_CHILD(_index, _offset, _parent, _depth, _first_child, \
			    ...)                                             \
	if (nodeoffset == (_offset))                                         \
		return (_first_child);
	__DT2C_FOREACH_NODE(__DT2C_FIRST_CHILD)
#undef __DT2C_FIRST_CHILD
	return -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE int dt2c_fdt_next_subnode(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_NEXT_SIBLING(_index, _offset, _parent, _depth,             \
			     _first_child, _next_sibling, ...)               \
	if (nodeoffset == (_offset))                                         \
		return (_next_sibling);
	__DT2C_FOREACH_NODE(__DT2C_NEXT_SIBLING)
#undef __DT2C_NEXT_SIBLING
	return -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE int dt2c_fdt_subnode_offset_namelen(const void *fdt,
					    int parentoffset,
					    const char *name, int namelen)
{
	int child;

	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (!__dt2c_node_valid(parentoffset))
		return -DT2C_FDT_ERR_BADOFFSET;
	for (child = dt2c_fdt_first_subnode(fdt, parentoffset); child >= 0;
	     child = dt2c_fdt_next_subnode(fdt, child)) {
		const char *node_name = dt2c_fdt_get_name(fdt, child, NULL);

		if (__dt2c_node_name_equal(node_name, name, namelen))
			return child;
	}
	return child;
}

__DT2C_INLINE int dt2c_fdt_subnode_offset(const void *fdt, int parentoffset,
				    const char *name)
{
	return dt2c_fdt_subnode_offset_namelen(fdt, parentoffset, name,
					  (int)__builtin_strlen(name));
}

__DT2C_INLINE const char *dt2c_fdt_get_alias_namelen(const void *fdt,
					       const char *name, int namelen)
{
	int aliases = dt2c_fdt_subnode_offset(fdt, 0, "aliases");

	return aliases < 0 ? NULL :
		(const char *)dt2c_fdt_getprop_namelen(fdt, aliases, name,
						  namelen, NULL);
}

__DT2C_INLINE const char *dt2c_fdt_get_alias(const void *fdt, const char *name)
{
	return dt2c_fdt_get_alias_namelen(fdt, name, (int)__builtin_strlen(name));
}

/* Resolve fixed aliases against generated node paths without walking the tree. */
__DT2C_INLINE int dt2c_fdt_alias_node_offset(const void *fdt, const char *name)
{
	const char *path;
	int aliases;
	int length;

	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	aliases = dt2c_fdt_subnode_offset(fdt, 0, "aliases");
	if (aliases < 0)
		return aliases;
	path = (const char *)dt2c_fdt_getprop_namelen(
		fdt, aliases, name, (int)__builtin_strlen(name), &length);
	if (!path)
		return -DT2C_FDT_ERR_NOTFOUND;
	if (length <= 0 || path[length - 1] != '\0')
		return -DT2C_FDT_ERR_BADPATH;
#define __DT2C_ALIAS_NODE(_index, _offset, _parent, _depth, _first_child, \
			   _next_sibling, _first_property, _property_count, \
			   _next_node, _next_delta, _name, _path, _phandle) \
	if (__dt2c_namelen_equal(path, length - 1, (_path)))                 \
		return (_offset);
	__DT2C_FOREACH_NODE(__DT2C_ALIAS_NODE)
#undef __DT2C_ALIAS_NODE
	return -DT2C_FDT_ERR_NOTFOUND;
}

static inline int __dt2c_absolute_path_offset(const void *fdt,
					      const char *path,
					      const char *end,
					      int offset)
{
	while (path < end) {
		const char *slash;

		while (path < end && *path == '/')
			++path;
		if (path == end)
			return offset;
		slash = (const char *)__builtin_memchr(
			path, '/', (size_t)(end - path));
		if (!slash)
			slash = end;
		offset = dt2c_fdt_subnode_offset_namelen(
			fdt, offset, path, (int)(slash - path));
		if (offset < 0)
			return offset;
		path = slash;
	}
	return offset;
}

__DT2C_INLINE int dt2c_fdt_path_offset_namelen(const void *fdt, const char *path,
					 int namelen)
{
	const char *end;

	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (namelen < 0)
		return -DT2C_FDT_ERR_BADPATH;
#define __DT2C_PATH_MATCH(_index, _offset, _parent, _depth, _first_child,  \
			   _next_sibling, _first_property, _property_count, \
			   _next_node, _next_delta, _name, _path, _phandle) \
	if (__dt2c_namelen_equal(path, namelen, (_path)))                     \
		return (_offset);
	__DT2C_FOREACH_NODE(__DT2C_PATH_MATCH)
#undef __DT2C_PATH_MATCH
	end = path + namelen;
	if (path == end)
		return -DT2C_FDT_ERR_BADPATH;
	if (*path != '/') {
		const char *slash = (const char *)__builtin_memchr(
			path, '/', (size_t)(end - path));
		const char *alias;
		int alias_length;
		int aliases;
		int offset;

		if (!slash)
			slash = end;
		aliases = dt2c_fdt_subnode_offset(fdt, 0, "aliases");
		alias = aliases < 0 ? NULL :
			(const char *)dt2c_fdt_getprop_namelen(
				fdt, aliases, path, (int)(slash - path),
				&alias_length);
		if (!alias || alias_length <= 0 || alias[alias_length - 1] != '\0')
			return -DT2C_FDT_ERR_BADPATH;
		offset = __dt2c_absolute_path_offset(
			fdt, alias,
			alias + __dt2c_strnlen(alias, (size_t)alias_length), 0);
		if (offset < 0)
			return offset;
		return __dt2c_absolute_path_offset(fdt, slash, end, offset);
	}
	return __dt2c_absolute_path_offset(fdt, path, end, 0);
}

__DT2C_INLINE int dt2c_fdt_path_offset(const void *fdt, const char *path)
{
	return dt2c_fdt_path_offset_namelen(fdt, path,
					(int)__builtin_strlen(path));
}

__DT2C_INLINE int dt2c_fdt_get_path(const void *fdt, int nodeoffset, char *buf,
			      int buflen)
{
	const char *path;
	size_t length;

	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_GET_PATH(_index, _offset, _parent, _depth, _first_child,    \
			 _next_sibling, _first_property, _property_count,  \
			 _next_node, _next_delta, _name, _path, _phandle)  \
	if (nodeoffset == (_offset))                                         \
		path = (_path);
	path = NULL;
	__DT2C_FOREACH_NODE(__DT2C_GET_PATH)
#undef __DT2C_GET_PATH
	if (!path)
		return -DT2C_FDT_ERR_BADOFFSET;
	length = __builtin_strlen(path) + 1;
	if (buflen < 0 || (size_t)buflen < length)
		return -DT2C_FDT_ERR_NOSPACE;
	__builtin_memcpy(buf, path, length);
	return 0;
}

__DT2C_INLINE int dt2c_fdt_node_depth(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_NODE_DEPTH(_index, _offset, _parent, _depth, ...)           \
	if (nodeoffset == (_offset))                                         \
		return (_depth);
	__DT2C_FOREACH_NODE(__DT2C_NODE_DEPTH)
#undef __DT2C_NODE_DEPTH
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE int dt2c_fdt_parent_offset(const void *fdt, int nodeoffset)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
#define __DT2C_PARENT_OFFSET(_index, _offset, _parent, ...)                \
	if (nodeoffset == (_offset))                                         \
		return (_parent);
	__DT2C_FOREACH_NODE(__DT2C_PARENT_OFFSET)
#undef __DT2C_PARENT_OFFSET
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE int dt2c_fdt_supernode_atdepth_offset(const void *fdt,
					      int nodeoffset,
					      int supernodedepth,
					      int *nodedepth)
{
	int depth = dt2c_fdt_node_depth(fdt, nodeoffset);

	if (depth < 0)
		return depth;
	if (nodedepth)
		*nodedepth = depth;
	if (supernodedepth < 0 || supernodedepth > depth)
		return -DT2C_FDT_ERR_NOTFOUND;
	while (depth-- > supernodedepth)
		nodeoffset = dt2c_fdt_parent_offset(fdt, nodeoffset);
	return nodeoffset;
}

__DT2C_INLINE uint32_t dt2c_fdt_get_phandle(const void *fdt, int nodeoffset)
{
	const dt2c_fdt32_t *value;
	int length;

	value = (const dt2c_fdt32_t *)dt2c_fdt_getprop(fdt, nodeoffset, "phandle",
					    &length);
	if (!value || length != (int)sizeof(*value))
		value = (const dt2c_fdt32_t *)dt2c_fdt_getprop(
			fdt, nodeoffset, "linux,phandle", &length);
	return value && length == (int)sizeof(*value) ?
		       dt2c_fdt32_to_cpu(*value) : 0;
}

__DT2C_INLINE int dt2c_fdt_node_offset_by_phandle(const void *fdt,
					    uint32_t phandle)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (phandle == 0 || phandle == (uint32_t)-1)
		return -DT2C_FDT_ERR_BADPHANDLE;
#define __DT2C_PHANDLE_MATCH(_index, _offset, _parent, _depth,             \
			      _first_child, _next_sibling, _first_property,  \
			      _property_count, _next_node, _next_delta,      \
			      _name, _path, _phandle)                        \
	if ((_phandle) == phandle)                                           \
		return (_offset);
	__DT2C_FOREACH_NODE(__DT2C_PHANDLE_MATCH)
#undef __DT2C_PHANDLE_MATCH
	return -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE int dt2c_fdt_find_max_phandle(const void *fdt, uint32_t *phandle)
{
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (phandle)
		*phandle = __DT2C_FDT_MAX_PHANDLE;
	return 0;
}

__DT2C_INLINE int dt2c_fdt_generate_phandle(const void *fdt, uint32_t *phandle)
{
	uint32_t maximum;
	int error = dt2c_fdt_find_max_phandle(fdt, &maximum);

	if (error < 0)
		return error;
	if (maximum == DT2C_FDT_MAX_PHANDLE)
		return -DT2C_FDT_ERR_NOPHANDLES;
	if (phandle)
		*phandle = maximum + 1;
	return 0;
}

__DT2C_INLINE int dt2c_fdt_stringlist_contains(const char *strlist, int listlen,
					 const char *string)
{
	size_t wanted = __builtin_strlen(string);

	while (listlen > 0) {
		size_t length = __dt2c_strnlen(strlist, (size_t)listlen);

		if (length == (size_t)listlen)
			return 0;
		if (length == wanted &&
		    __builtin_memcmp(strlist, string, wanted) == 0)
			return 1;
		strlist += length + 1;
		listlen -= (int)length + 1;
	}
	return 0;
}

__DT2C_INLINE int dt2c_fdt_stringlist_count(const void *fdt, int nodeoffset,
				      const char *property)
{
	const char *list;
	int count = 0;
	int length;

	list = (const char *)dt2c_fdt_getprop(fdt, nodeoffset, property, &length);
	if (!list)
		return length;
	while (length > 0) {
		size_t item = __dt2c_strnlen(list, (size_t)length);

		if (item == (size_t)length)
			return -DT2C_FDT_ERR_BADVALUE;
		list += item + 1;
		length -= (int)item + 1;
		++count;
	}
	return count;
}

__DT2C_INLINE int dt2c_fdt_stringlist_search(const void *fdt, int nodeoffset,
				       const char *property,
				       const char *string)
{
	const char *list;
	int index = 0;
	int length;
	size_t wanted = __builtin_strlen(string);

	list = (const char *)dt2c_fdt_getprop(fdt, nodeoffset, property, &length);
	if (!list)
		return length;
	while (length > 0) {
		size_t item = __dt2c_strnlen(list, (size_t)length);

		if (item == (size_t)length)
			return -DT2C_FDT_ERR_BADVALUE;
		if (item == wanted &&
		    __builtin_memcmp(list, string, wanted) == 0)
			return index;
		list += item + 1;
		length -= (int)item + 1;
		++index;
	}
	return -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE const char *dt2c_fdt_stringlist_get(const void *fdt, int nodeoffset,
					    const char *property,
					    int index, int *lenp)
{
	const char *list;
	int length;

	list = (const char *)dt2c_fdt_getprop(fdt, nodeoffset, property, &length);
	if (!list) {
		if (lenp)
			*lenp = length;
		return NULL;
	}
	while (length > 0) {
		size_t item = __dt2c_strnlen(list, (size_t)length);

		if (item == (size_t)length) {
			if (lenp)
				*lenp = -DT2C_FDT_ERR_BADVALUE;
			return NULL;
		}
		if (index == 0) {
			if (lenp)
				*lenp = (int)item;
			return list;
		}
		list += item + 1;
		length -= (int)item + 1;
		--index;
	}
	if (lenp)
		*lenp = -DT2C_FDT_ERR_NOTFOUND;
	return NULL;
}

__DT2C_INLINE int dt2c_fdt_node_check_compatible(const void *fdt,
					   int nodeoffset,
					   const char *compatible)
{
	int found = 0;
	(void)compatible;

	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (!__dt2c_node_valid(nodeoffset))
		return -DT2C_FDT_ERR_BADOFFSET;
#define __DT2C_COMPATIBLE_NODE(_offset)                                   \
	if (nodeoffset == (_offset))                                         \
		found = 1;
	__DT2C_FOREACH_COMPATIBLE_NODE(__DT2C_COMPATIBLE_NODE)
#undef __DT2C_COMPATIBLE_NODE
#define __DT2C_COMPATIBLE_CHECK(_offset, _compatible)                       \
	if (nodeoffset == (_offset) &&                                       \
	    __builtin_strcmp(compatible, (_compatible)) == 0)                \
		return 0;
	__DT2C_FOREACH_COMPATIBLE(__DT2C_COMPATIBLE_CHECK)
#undef __DT2C_COMPATIBLE_CHECK
	return found ? 1 : -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE int dt2c_fdt_node_offset_by_compatible(const void *fdt,
					       int startoffset,
					       const char *compatible)
{
	(void)compatible;
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (startoffset >= 0 && !__dt2c_node_valid(startoffset))
		return -DT2C_FDT_ERR_BADOFFSET;
#define __DT2C_COMPATIBLE_FIND(_offset, _compatible)                        \
	if (startoffset < (_offset) &&                                       \
	    __builtin_strcmp(compatible, (_compatible)) == 0)                \
		return (_offset);
	__DT2C_FOREACH_COMPATIBLE(__DT2C_COMPATIBLE_FIND)
#undef __DT2C_COMPATIBLE_FIND
	return -DT2C_FDT_ERR_NOTFOUND;
}

__DT2C_INLINE int dt2c_fdt_node_offset_by_prop_value(const void *fdt,
					       int startoffset,
					       const char *propname,
					       const void *propval,
					       int proplen)
{
	int offset;

	for (offset = dt2c_fdt_next_node(fdt, startoffset, NULL); offset >= 0;
	     offset = dt2c_fdt_next_node(fdt, offset, NULL)) {
		const void *value;
		int length;

		value = dt2c_fdt_getprop(fdt, offset, propname, &length);
		if (value && length == proplen &&
		    __builtin_memcmp(value, propval, (size_t)length) == 0)
			return offset;
	}
	return offset;
}

__DT2C_INLINE int dt2c_fdt_num_mem_rsv(const void *fdt)
{
	return __dt2c_fdt_valid(fdt) ? __DT2C_FDT_RESERVATION_COUNT :
				       -DT2C_FDT_ERR_BADMAGIC;
}

__DT2C_INLINE int dt2c_fdt_get_mem_rsv(const void *fdt, int index,
				 uint64_t *address, uint64_t *size)
{
	(void)address;
	(void)size;
	if (!__dt2c_fdt_valid(fdt))
		return -DT2C_FDT_ERR_BADMAGIC;
	if (index < 0 || index > __DT2C_FDT_RESERVATION_COUNT)
		return -DT2C_FDT_ERR_BADOFFSET;
	if (index == __DT2C_FDT_RESERVATION_COUNT) {
		*address = 0;
		*size = 0;
		return 0;
	}
#define __DT2C_RESERVATION_MATCH(_index, _address, _size)                  \
	if (index == (_index)) {                                              \
		*address = (_address);                                         \
		*size = (_size);                                               \
		return 0;                                                      \
	}
	__DT2C_FOREACH_RESERVATION(__DT2C_RESERVATION_MATCH)
#undef __DT2C_RESERVATION_MATCH
	return -DT2C_FDT_ERR_BADOFFSET;
}

__DT2C_INLINE int __dt2c_cells(const void *fdt, int nodeoffset,
			      const char *name)
{
	const dt2c_fdt32_t *value;
	uint32_t cells;
	int length;

	value = (const dt2c_fdt32_t *)dt2c_fdt_getprop(fdt, nodeoffset, name, &length);
	if (!value)
		return length;
	if (length != (int)sizeof(*value))
		return -DT2C_FDT_ERR_BADNCELLS;
	cells = dt2c_fdt32_to_cpu(*value);
	return cells > DT2C_FDT_MAX_NCELLS ? -DT2C_FDT_ERR_BADNCELLS : (int)cells;
}

__DT2C_INLINE int dt2c_fdt_address_cells(const void *fdt, int nodeoffset)
{
	int cells = __dt2c_cells(fdt, nodeoffset, "#address-cells");

	if (cells == 0)
		return -DT2C_FDT_ERR_BADNCELLS;
	return cells == -DT2C_FDT_ERR_NOTFOUND ? 2 : cells;
}

__DT2C_INLINE int dt2c_fdt_size_cells(const void *fdt, int nodeoffset)
{
	int cells = __dt2c_cells(fdt, nodeoffset, "#size-cells");

	return cells == -DT2C_FDT_ERR_NOTFOUND ? 1 : cells;
}

__DT2C_INLINE const char *dt2c_fdt_strerror(int error)
{
	static const char *const errors[] = {
		[0] = "<no error>",
		[DT2C_FDT_ERR_NOTFOUND] = "FDT_ERR_NOTFOUND",
		[DT2C_FDT_ERR_EXISTS] = "FDT_ERR_EXISTS",
		[DT2C_FDT_ERR_NOSPACE] = "FDT_ERR_NOSPACE",
		[DT2C_FDT_ERR_BADOFFSET] = "FDT_ERR_BADOFFSET",
		[DT2C_FDT_ERR_BADPATH] = "FDT_ERR_BADPATH",
		[DT2C_FDT_ERR_BADPHANDLE] = "FDT_ERR_BADPHANDLE",
		[DT2C_FDT_ERR_BADSTATE] = "FDT_ERR_BADSTATE",
		[DT2C_FDT_ERR_TRUNCATED] = "FDT_ERR_TRUNCATED",
		[DT2C_FDT_ERR_BADMAGIC] = "FDT_ERR_BADMAGIC",
		[DT2C_FDT_ERR_BADVERSION] = "FDT_ERR_BADVERSION",
		[DT2C_FDT_ERR_BADSTRUCTURE] = "FDT_ERR_BADSTRUCTURE",
		[DT2C_FDT_ERR_BADLAYOUT] = "FDT_ERR_BADLAYOUT",
		[DT2C_FDT_ERR_INTERNAL] = "FDT_ERR_INTERNAL",
		[DT2C_FDT_ERR_BADNCELLS] = "FDT_ERR_BADNCELLS",
		[DT2C_FDT_ERR_BADVALUE] = "FDT_ERR_BADVALUE",
		[DT2C_FDT_ERR_BADOVERLAY] = "FDT_ERR_BADOVERLAY",
		[DT2C_FDT_ERR_NOPHANDLES] = "FDT_ERR_NOPHANDLES",
		[DT2C_FDT_ERR_BADFLAGS] = "FDT_ERR_BADFLAGS",
		[DT2C_FDT_ERR_ALIGNMENT] = "FDT_ERR_ALIGNMENT",
	};

	if (error > 0)
		return "<valid offset/length>";
	if (error == 0)
		return errors[0];
	if (-error <= DT2C_FDT_ERR_MAX && errors[-error])
		return errors[-error];
	return "<unknown error>";
}

#undef __DT2C_INLINE

#endif /* DT2C_DT_RUNTIME_H */
