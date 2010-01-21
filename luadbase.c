/**
 * luamysql - MYSQL driver
 * (c) 2009-19 Alacner zhang <alacner@gmail.com>
 * This content is released under the MIT License.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define VERSION "1.0.0"

#include "dbf.h"

#include <fcntl.h>
#include <errno.h>

#include "lua.h"
#include "lauxlib.h"
#if ! defined (LUA_VERSION_NUM) || LUA_VERSION_NUM < 501
#include "compat-5.1.h"
#endif

/* Compatibility between Lua 5.1+ and Lua 5.0 */
#ifndef LUA_VERSION_NUM
#define LUA_VERSION_NUM 0
#endif
#if LUA_VERSION_NUM < 501
#define luaL_register(a, b, c) luaL_openlib((a), (b), (c), 0)
#endif

#define emalloc(a) malloc((a))
#define erealloc(a, b) realloc((a), (b))
#define efree(a) free((a))
#define LUA_DBASE_CONN "Dbase connection"

typedef struct {
    short      closed;
} pseudo_data;

typedef struct {
	short closed;
	dbhead_t *dbh;
} lua_dbase_dbh;

void luaM_setmeta (lua_State *L, const char *name);
int luaM_register (lua_State *L, const char *name, const luaL_reg *methods);
int luaopen_dbase (lua_State *L);

/**                   
* Return the name of the object's metatable.
* This function is used by `tostring'.     
*/
static int luaM_tostring (lua_State *L) {
    char buff[100];
    pseudo_data *obj = (pseudo_data *)lua_touserdata (L, 1);
    if (obj->closed)
        strcpy (buff, "closed");
    else
        sprintf (buff, "%p", (void *)obj);
    lua_pushfstring (L, "%s (%s)", lua_tostring(L,lua_upvalueindex(1)), buff);
    return 1;
}

void luaM_regconst(lua_State *L, const char *name, long value) {
    lua_pushstring(L, name);
    lua_pushnumber(L, value);
    lua_rawset(L, -3);
}


/**
* Define the metatable for the object on top of the stack
*/
void luaM_setmeta (lua_State *L, const char *name) {
    luaL_getmetatable (L, name);
    lua_setmetatable (L, -2);
}

/**
* Create a metatable and leave it on top of the stack.
*/
int luaM_register (lua_State *L, const char *name, const luaL_reg *methods) {
    if (!luaL_newmetatable (L, name))
        return 0;

    /* define methods */
    luaL_register (L, NULL, methods);

    /* define metamethods */
    lua_pushliteral (L, "__gc");
    lua_pushcfunction (L, methods->func);
    lua_settable (L, -3);

    lua_pushliteral (L, "__index");
    lua_pushvalue (L, -2);
    lua_settable (L, -3);

    lua_pushliteral (L, "__tostring");
    lua_pushstring (L, name);
    lua_pushcclosure (L, luaM_tostring, 1);
    lua_settable (L, -3);

    lua_pushliteral (L, "__metatable");
    lua_pushliteral (L, "you're not allowed to get this metatable");
    lua_settable (L, -3);

    return 1;
}

static lua_dbase_dbh *Mget_dbh (lua_State *L) {
	lua_dbase_dbh *my_dbh = (lua_dbase_dbh *)luaL_checkudata (L, 1, LUA_DBASE_CONN);
    luaL_argcheck (L, my_dbh != NULL, 1, "connection expected");
    luaL_argcheck (L, !my_dbh->closed, 1, "connection is closed");
    return my_dbh;
}

static int Ldbase_open (lua_State *L) {
	dbhead_t *dbh;
    const char *dbf_filename = lua_tostring(L, 1);
    int mode = luaL_optnumber(L, 2, 0);

    dbh = dbf_open((char *)dbf_filename, mode);
    if (dbh == NULL) {
		lua_pushboolean(L, 0);
        lua_pushfstring(L, "unable to open database %s", dbf_filename);
		return 2;
    }
		
	lua_dbase_dbh *my_dbh = (lua_dbase_dbh *)lua_newuserdata(L, sizeof(lua_dbase_dbh));
	luaM_setmeta(L, LUA_DBASE_CONN);

	my_dbh->closed = 0;
	my_dbh->dbh = dbh;

	return 1;
}

static int Ldbase_numfields (lua_State *L) {
	lua_pushnumber(L, Mget_dbh(L)->dbh->db_nfields);
	return 1;
}

static int Ldbase_numrecords (lua_State *L) {
	lua_pushnumber(L, Mget_dbh(L)->dbh->db_records);
	return 1;
}

static int Ldbase_pack (lua_State *L) {
    lua_dbase_dbh *my_dbh = Mget_dbh (L);
	pack_dbf(my_dbh->dbh);
    put_dbf_info(my_dbh->dbh);

	lua_pushboolean(L, 1);
	return 1;
}

static int Ldbase_get_header_info(lua_State *L) {
    dbfield_t   *dbf, *cur_f;
    dbhead_t    *dbh;

    lua_dbase_dbh *my_dbh = Mget_dbh (L);
	dbh = my_dbh->dbh;

	lua_newtable(L);
	
    dbf = dbh->db_fields;
	int i = 0;
    for (cur_f = dbf; cur_f < &dbh->db_fields[dbh->db_nfields]; ++cur_f) {
		lua_newtable(L);

        /* field name */
		lua_pushstring(L, "name");
		lua_pushstring(L, cur_f->db_fname);
		lua_rawset(L, -3);

        /* field type */
		char *type = NULL;
		lua_pushstring(L, "type");
        switch (cur_f->db_type) {
            case 'C': type = "character";    break;
            case 'D': type = "date";         break;
            case 'I': type = "integer";      break;
            case 'N': type = "number";       break;
            case 'L': type = "boolean";      break;
            case 'M': type = "memo";         break;
            case 'F': type = "float";     break;
            default:  type = "unknown";      break;
        }
		lua_pushstring(L, type);
		lua_rawset(L, -3);

        /* length of field */
		lua_pushstring(L, "length");
		lua_pushnumber(L, cur_f->db_flen);
		lua_rawset(L, -3);

        /* number of decimals in field */
		lua_pushstring(L, "precision");
		int precision = -1;
        switch (cur_f->db_type) {
            case 'N':
            case 'I':
                precision = cur_f->db_fdc;
                break;
            default:
                precision = 0;
        }
		lua_pushnumber(L, precision);
		lua_rawset(L, -3);

        /* format for printing %s etc */
		lua_pushstring(L, "format");
		lua_pushstring(L, cur_f->db_format);
		lua_rawset(L, -3);

        /* offset within record */
		lua_pushstring(L, "offset");
		lua_pushnumber(L, cur_f->db_foffset);
		lua_rawset(L, -3);

		lua_rawseti(L, -2, i);
		i++;
    }

	return 1;
}

static int dbase_get_record (lua_State *L, int assoc) {
    lua_dbase_dbh *my_dbh = Mget_dbh (L);

    dbfield_t *dbf, *cur_f;
    char *data, *fnp, *str_value;
    size_t cursize = 0;
    long overflow_test;
    int errno_save;
	
	dbhead_t *dbh = my_dbh->dbh;

	lua_Number record = lua_tonumber(L, 2);	

    if ((data = get_dbf_record(dbh, record)) == NULL) {
        lua_pushfstring(L, "Tried to read bad record %d", record);
		lua_pushboolean(L, 0);
		return 1;
    }

	dbf = dbh->db_fields;

	lua_newtable(L);

    fnp = NULL;
	int i = 1;
    for (cur_f = dbf; cur_f < &dbf[dbh->db_nfields]; cur_f++) {
        /* get the value */
        str_value = (char *)emalloc(cur_f->db_flen + 1);

        if(cursize <= (unsigned)cur_f->db_flen) {
            cursize = cur_f->db_flen + 1;
            fnp = erealloc(fnp, cursize);
        }

        snprintf(str_value, cursize, cur_f->db_format, get_field_val(data, cur_f, fnp));

        /* now convert it to the right php internal type */
        switch (cur_f->db_type) {
            case 'C':
            case 'D':
                if (!assoc) {
					lua_pushstring(L, str_value);
					lua_rawseti(L, -2, i);
					i++;
                } else {
					lua_pushstring(L, cur_f->db_fname);
					lua_pushstring(L, str_value);
					lua_rawset(L, -3);
                }
                break;
            case 'I':   /* FALLS THROUGH */
            case 'N':
                if (cur_f->db_fdc == 0) {
                    /* Large integers in dbase can be larger than long */
                    errno_save = errno;
                    overflow_test = strtol(str_value, NULL, 10);
                    if (errno == ERANGE) {
                        /* If the integer is too large, keep it as string */
						if (!assoc) {
							lua_pushstring(L, str_value);
							lua_rawseti(L, -2, i);
							i++;
						} else {
							lua_pushstring(L, cur_f->db_fname);
							lua_pushstring(L, str_value);
							lua_rawset(L, -3);
						}
                    } else {
                        if (!assoc) {
							lua_pushnumber(L, overflow_test);
							lua_rawseti(L, -2, i);
							i++;
                        } else {
							lua_pushstring(L, cur_f->db_fname);
							lua_pushnumber(L, overflow_test);
							lua_rawset(L, -3);
                        }
                    }
                    errno = errno_save;
                } else {
					if (!assoc) {
						lua_pushnumber(L, atof(str_value));
						lua_rawseti(L, -2, i);
						i++;
					} else {
						lua_pushstring(L, cur_f->db_fname);
						lua_pushnumber(L, atof(str_value));
						lua_rawset(L, -3);
					}
                }
                break;
            case 'F':
				if (!assoc) {
					lua_pushnumber(L, atof(str_value));
					lua_rawseti(L, -2, i);
					i++;
				} else {
					lua_pushstring(L, cur_f->db_fname);
					lua_pushnumber(L, atof(str_value));
					lua_rawset(L, -3);
				}
                break;
            case 'L':   /* we used to FALL THROUGH, but now we check for T/Y and F/N
                           and insert 1 or 0, respectively.  db_fdc is the number of
                           decimals, which we don't care about.      3/14/2001 LEW */
                if ((*str_value == 'T') || (*str_value == 'Y')) {
					if (!assoc) {
						lua_pushnumber(L, strtol("1", NULL, 10));
						lua_rawseti(L, -2, i);
						i++;
					} else {
						lua_pushstring(L, cur_f->db_fname);
						lua_pushnumber(L, strtol("1", NULL, 10));
						lua_rawset(L, -3);
					}
                } else {
                    if ((*str_value == 'F') || (*str_value == 'N')) {
						if (!assoc) {
							lua_pushnumber(L, strtol("0", NULL, 10));
							lua_rawseti(L, -2, i);
							i++;
						} else {
							lua_pushstring(L, cur_f->db_fname);
							lua_pushnumber(L, strtol("0", NULL, 10));
							lua_rawset(L, -3);
						}
                    } else {
						if (!assoc) {
							lua_pushnumber(L, strtol(" ", NULL, 10));
							lua_rawseti(L, -2, i);
							i++;
						} else {
							lua_pushstring(L, cur_f->db_fname);
							lua_pushnumber(L, strtol(" ", NULL, 10));
							lua_rawset(L, -3);
						}
                    }
                }
                break;
            case 'M':
                /* this is a memo field. don't know how to deal with this yet */
                break;
            default:
                /* should deal with this in some way */
                break;
        }
        efree(str_value);
    }

    efree(fnp);

    /* mark whether this record was deleted */
    if (data[0] == '*') {
		lua_pushstring(L, "deleted");
		lua_pushnumber(L, 1);
		lua_rawset(L, -3);
    } else {
		lua_pushstring(L, "deleted");
		lua_pushnumber(L, 0);
		lua_rawset(L, -3);
    }

    free(data);

	return 1;
}

static int Ldbase_get_record (lua_State *L) {
    return dbase_get_record(L, 0);
}

static int Ldbase_get_record_with_names (lua_State *L) {
    return dbase_get_record(L, 1);
}

static int Ldbase_delete_record (lua_State *L) {
    lua_dbase_dbh *my_dbh = Mget_dbh (L);

	dbhead_t *dbh = my_dbh->dbh;

	lua_Number record = lua_tonumber(L, 2);	

    if (del_dbf_record(dbh, record) < 0) {
		lua_pushboolean(L, 0);
        if (record > dbh->db_records) {
            lua_pushfstring(L, "record %d out of bounds", record);
        } else {
            lua_pushfstring(L, "unable to delete record %d", record);
        }
		return 2;
    }

    put_dbf_info(dbh);
	lua_pushboolean(L, 1);
	return 1;
}

static int Ldbase_modify_record (lua_State *L) {
    lua_dbase_dbh *my_dbh = Mget_dbh (L);

	dbhead_t *dbh = my_dbh->dbh;

    int num_fields;
    dbfield_t *dbf, *cur_f;
    char *cp, *t_cp;
    int i;

	lua_Number record = -1;
	if (lua_isnumber(L, -1)) {
		record = lua_tonumber(L, -1);	
		lua_pop(L, 1);
	}

    if ( ! lua_istable(L, -1)) {
		lua_pushboolean(L, 0);
        lua_pushstring(L, "Expected array as first parameter");
		return 2;
    }

	lua_newtable(L);

	lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
		lua_tostring(L, -1);
		lua_tostring(L, -2);

		lua_rawset(L, -3);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

	num_fields = lua_objlen(L, -1);

    if (num_fields != dbh->db_nfields) {
		lua_pushboolean(L, 0);
        lua_pushstring(L, "Wrong number of fields specified, Index num must start 1, not 0.");
		return 2;
    }

    cp = t_cp = (char *)emalloc(dbh->db_rlen + 1);
    *t_cp++ = VALID_RECORD;

    dbf = dbh->db_fields;
    for (i = 1, cur_f = dbf; cur_f < &dbf[num_fields]; i++, cur_f++) {
		lua_rawgeti(L, -1, i); 
		snprintf(t_cp, cur_f->db_flen+1, cur_f->db_format, lua_tostring(L, -1));
        t_cp += cur_f->db_flen;
		lua_pop(L, 1);
    }

	if (record == -1) {
		dbh->db_records++;
		record = dbh->db_records;
	}

    if (put_dbf_record(dbh, record, cp) < 0) {
		lua_pushboolean(L, 0);
        lua_pushfstring(L, "unable to put record at %d", dbh->db_records);
        efree(cp);
		return 2;
    }

    put_dbf_info(dbh);
    efree(cp);

	lua_pushboolean(L, 1);
	lua_pushnumber(L, dbh->db_records);
	return 2;
}

static int Ldbase_create (lua_State *L) {
    int fd;
    dbhead_t *dbh;

    int num_fields;
    dbfield_t *dbf, *cur_f;
    int i, rlen;

    const char *filename = lua_tostring(L, 1);

    if ((fd = open(filename, O_BINARY|O_RDWR|O_CREAT, 0644)) < 0) {
		lua_pushboolean(L, 0);
        lua_pushfstring(L,"Unable to create database (%d): %s", errno, strerror(errno));
		return 2;
    }

	// Each field consists of a name, a character indicating the field type, and optionally, a length, and a precision.
	// The fieldnames are limited in length and must not exceed 10 chars. 
    if ( ! lua_istable(L, 2)) {
		lua_pushboolean(L, 0);
        lua_pushstring(L, "Expected array as second parameter");
		return 2;
    }

	lua_newtable(L);

	lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
		lua_tostring(L, -1);
		lua_tostring(L, -2);

		lua_rawset(L, -3);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

	num_fields = lua_objlen(L, -1);


    if (num_fields <= 0) {
		lua_pushboolean(L, 0);
        lua_pushstring(L, "Unable to create database without fields");
		return 2;
    }

    /* have to use regular malloc() because this gets free()d by
       code in the dbase library */
    dbh = (dbhead_t *)malloc(sizeof(dbhead_t));
    dbf = (dbfield_t *)malloc(sizeof(dbfield_t) * num_fields);
    if (!dbh || !dbf) {
		lua_pushboolean(L, 0);
        lua_pushstring(L, "Unable to allocate memory for header info");
		return 2;
    }
    /* initialize the header structure */
    dbh->db_fields = dbf;
    dbh->db_fd = fd;
    dbh->db_dbt = DBH_TYPE_NORMAL;
    strcpy(dbh->db_date, "19930818");
    dbh->db_records = 0;
    dbh->db_nfields = num_fields;
    dbh->db_hlen = sizeof(struct dbf_dhead) + 1 + num_fields * sizeof(struct dbf_dfield);

    rlen = 1;
    /**
     * Patch by greg@darkphoton.com
     **/
    /* make sure that the db_format entries for all fields are set to NULL to ensure we
       don't seg fault if there's an error and we need to call free_dbf_head() before all
       fields have been defined. */
    for (i = 0, cur_f = dbf; i < num_fields; i++, cur_f++) {
        cur_f->db_format = NULL;
    }
    /**
     * end patch
     */

    for (i = 0, cur_f = dbf; i < num_fields; i++, cur_f++) {
        /* look up the first field */
		lua_rawgeti(L, -1, i+1); 
		if ( !lua_istable(L, -1)) {
			lua_pushboolean(L, 0);
            lua_pushfstring(L, "unable to find field %d", i+1);
            free_dbf_head(dbh);
			return 2;
        }

        /* field name */
		lua_rawgeti(L, -1, 1); 
		char *field_name = (char *)lua_tostring(L, -1); 
		lua_pop(L, 1);
        if (strlen(field_name) > 10 || strlen(field_name) == 0) {
			lua_pushboolean(L, 0);
            lua_pushfstring(L, "invalid field name '%s' (must be non-empty and less than or equal to 10 characters)", field_name);
            free_dbf_head(dbh);
			return 2;
        }
        copy_crimp(cur_f->db_fname, field_name, strlen(field_name));

        /* field type */
		lua_rawgeti(L, -1, 2); 
		char *field_type = (char *)lua_tostring(L, -1); 
		lua_pop(L, 1);

        cur_f->db_type = toupper(field_type[0]);



        cur_f->db_fdc = 0;

        /* verify the field length */
        switch (cur_f->db_type) {
        case 'L':
            cur_f->db_flen = 1;
            break;
        case 'M':
            cur_f->db_flen = 10;
            dbh->db_dbt = DBH_TYPE_MEMO;
            /* should create the memo file here, probably */
            break;
        case 'D':
            cur_f->db_flen = 8;
            break;
        case 'F':
            cur_f->db_flen = 20;
            break;
        case 'N':
        case 'C':
            /* field length */
			lua_rawgeti(L, -1, 3); 
			lua_Number field_length = lua_tonumber(L, -1); 
			lua_pop(L, 1);
            if ( !field_length) {
				lua_pushboolean(L, 0);
                lua_pushfstring(L, "expected field length as third element of list in field %d", i+1);
                free_dbf_head(dbh);
				return 2;
            }
            cur_f->db_flen = field_length;

            if (cur_f->db_type == 'N') {
				lua_rawgeti(L, -1, 4); 
				lua_Number field_precision = lua_tonumber(L, -1); 
				lua_pop(L, 1);
				if ( !field_length) {
					lua_pushboolean(L, 0);
                    lua_pushfstring(L, "expected field precision as fourth element of list in field %d", i+1);
                    free_dbf_head(dbh);
					return 2;
                }
                cur_f->db_fdc = field_precision;
            }
            break;
        default:
			lua_pushboolean(L, 0);
            lua_pushfstring(L, "unknown field type '%c'", cur_f->db_type);
            free_dbf_head(dbh);
			return 2;
        }
        cur_f->db_foffset = rlen;
        rlen += cur_f->db_flen;

        cur_f->db_format = get_dbf_f_fmt(cur_f);

		lua_pop(L, 1);
    }

		//lua_pushstring(L, field_type);
    dbh->db_rlen = rlen;
    put_dbf_info(dbh);

	//-----
	lua_dbase_dbh *my_dbh = (lua_dbase_dbh *)lua_newuserdata(L, sizeof(lua_dbase_dbh));
	luaM_setmeta(L, LUA_DBASE_CONN);

	my_dbh->closed = 0;
	my_dbh->dbh = dbh;

	return 1;

}

/**
* Close connection
*/
static int Ldbase_close (lua_State *L) {
    lua_dbase_dbh *my_dbh = Mget_dbh (L);
    luaL_argcheck (L, my_dbh != NULL, 1, "connection expected");
    if (my_dbh->closed) {
        lua_pushboolean (L, 0);
        return 1;
    }

    my_dbh->closed = 1;
    lua_pushboolean (L, 1);
    return 1;
}


static int Lversion (lua_State *L) {
    lua_pushfstring(L, "luadbase (%s) - dbase(dbf) driver\n", VERSION);
    lua_pushstring(L, "(c) 2009-19 Alacner zhang <alacner@gmail.com>\n");
    lua_pushstring(L, "This content is released under the MIT License.\n");

    lua_concat (L, 3);
    return 1;
}

int luaopen_dbase (lua_State *L) {
    struct luaL_reg driver[] = {
        { "version",   Lversion },
        { "create",   Ldbase_create },
        { "open",   Ldbase_open },
        { NULL, NULL },
    };

    struct luaL_reg dbh_methods[] = {
        { "numfields",   Ldbase_numfields },
        { "numrecords",   Ldbase_numrecords },
        { "pack", Ldbase_pack },
        { "get_record", Ldbase_get_record },
        { "get_record_with_names", Ldbase_get_record_with_names },
        { "get_header_info", Ldbase_get_header_info },
        { "delete_record", Ldbase_delete_record },
        { "add_record", Ldbase_modify_record },
        { "replace_record", Ldbase_modify_record },
        { "close",   Ldbase_close },
		{ NULL, NULL }
    };

    luaM_register (L, LUA_DBASE_CONN, dbh_methods);
	lua_pop (L, 1);

	luaL_register (L, "dbase", driver);
	return 1;
}
