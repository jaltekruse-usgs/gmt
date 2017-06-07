/*--------------------------------------------------------------------
 *	$Id$
 *
 *	Copyright (c) 1991-2017 by P. Wessel, W. H. F. Smith, R. Scharroo, J. Luis and F. Wobbe
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation; version 3 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	Contact info: gmt.soest.hawaii.edu
 *--------------------------------------------------------------------*/
/*
 * Public functions for the GMT C/C++ API.  The API consist of functions
 * in gmt_api.c, gmt_parse.c, and all the GMT modules; see gmt.h for list.
 *
 * Author: 	Paul Wessel
 * Date:	1-JUN-2013
 * Version:	5
 *
 * The API presently consists of 66 documented functions.  For a full
 * description of the API, see the GMT_API documentation.
 * These functions have Fortran bindings as well, provided you add
 * -DFORTRAN_API to the C preprocessor flags [in ConfigUser.cmake].
 *
 * There are 2 public functions used for GMT API session handling.
 * This part of the API helps the developer create and delete GMT sessions:
 *
 * GMT_Create_Session	  : Initialize a new GMT session
 * GMT_Destroy_Session	  : Destroy a GMT session
 *
 * There is 2 public functions for common error reporting.
 * Errors will be reported to stderr or selected log file:
 *
 * GMT_Message		      : Report an message given a verbosity level
 * GMT_Report		      : Report an error given an error code
 *
 * There are 27 further public functions used for GMT i/o activities:
 *
 * GMT_Alloc_Segment      : Allocate a single DATASET or TEXTSET segment
 * GMT_Begin_IO		      : Allow i/o to take place for rec-by-rec operations
 * GMT_Convert_Data       : Convert between different data sets, if possible
 * GMT_Create_Data	      : Return an empty container for a new data set
 * GMT_Destroy_Data	      : Destroy a data set and its container
 * GMT_Duplicate_Data     : Make an exact duplicate of a dataset
 * GMT_Duplicate_String   : Allocates a copy of a string to be freed by API
 * GMT_End_IO		      : Disallow further rec-by-rec i/o
 * GMT_Get_Record	      : Get the next single data record from the source(s)
 * GMT_Get_Row		      : Read one row from a grid
 * GMT_Get_Matrix	      : Get user matrix from GMT_MATRIX array
 * GMT_Get_Vector	      : Get user vector from GMT_VECTOR column
 * GMT_Init_IO		      : Initialize rec-by-rec i/o machinery before program use
 * GMT_Init_VirtualFile   : Reset a virtual file for reuse
 * GMT_Open_VirtualFile   : Open a memory location for reading or writing by a module
 * GMT_Put_Record	      : Send the next output record to its destination
 * GMT_Put_Row		      : Write one row to a grid
 * GMT_Put_Matrix	      : Hook user matrix to GMT_MATRIX array
 * GMT_Put_Vector	      : Hook user vector to GMT_VECTOR column
 * GMT_Read_Data	      : Load data into program memory from selected source
 * GMT_Read_Group	      : Read numerous files into an array of objects
 * GMT_Read_VirtualFile   : Obtain the memory resource that a module wrote to.
 * GMT_Register_IO	      : Register a source (or destination) for i/o use
 * GMT_Set_Comment	      : Update a comment for a data set
 * GMT_Status_IO	      : Exmine current status of record-by-record i/o
 * GMT_Write_Data	      : Place data set from program memory to selected destination
 * GMT_Encode_Options	  : Used by external APIs to fill out options from implicit rules

 * The above 28 functions deal with registration of input sources (files,
 * streams, file handles, or memory locations) and output destinations
 * (same flavors as input), the setup of the i/o, and generic functions
 * to access the data either in one go (GMT_Get|Put_Data) or on a
 * record-by-record basis (GMT_Get|Put_Record).  Finally, data sets that
 * are allocated can then be destroyed when no longer needed.
 *
 * There are 5 functions that deal with options, defaults and arguments:
 *
 * GMT_Get_Common	      : Checks for and returns values for GMT common options
 * GMT_Get_Default	      : Return the value of a GMT parameter as a string
 * GMT_Get_Values	      : Convert string to one or more coordinates or dimensions
 * GMT_Set_Default	      : Set a GMT parameter via a strings
 * GMT_Option		      : Display syntax for one or more GMT common options
 *
 * One function handles the listing of modules and the calling of any GMT module:
 *
 * GMT_Call_Module	: Call the specified GMT module
 *
 * Four functions are used to get grid index from row, col, and to obtain coordinates
 *
 * GMT_Get_Coord	      : Return array of coordinates for one dimension
 * GMT_Get_Index	      : Return 1-D grid index given row, col
 * GMT_Get_Pixel	      : Return 1-D image index given row, col, layer
 * GMT_Set_Columns        : Specify number of output columns for rec-by-rec writing
 *
 * For FFT operations there are 8 additional API functions:
 *
 * GMT_FFT		          : Call the forward or inverse FFT
 * GMT_FFT_1D		      : Lower-level 1-D FFT call
 * GMT_FFT_2D		      : Lower-level 2-D FFT call
 * GMT_FFT_Create	      : Initialize the FFT machinery for given dimension
 * GMT_FFT_Destroy	      : Destroy FFT machinery
 * GMT_FFT_Option	      : Display the syntax of the GMT FFT option settings
 * GMT_FFT_Parse	      : Parse the GMT FFT option
 * GMT_FFT_Wavenumber	  : Return selected wavenumber given its type
 *
 * There are also 13 functions for argument and option parsing.  See gmt_parse.c for these.
 *
 * Finally, three low-level F77-callable functions for grid i/o are given:
 *
 * gmt_f77_readgrdinfo_	  : Read the header of a GMT grid
 * gmt_f77_readgrd_		  : Read a GMT grid from file
 * gmt_f77_writegrd_	  : Write a GMT grid to file
 *
 * --------------------------------------------------------------------------------------------
 * Guru notes on memory management: Paul Wessel, June 2013.
 *
 * GMT maintains control over allocating, reallocating, and freeing of GMT objects created by GMT.
 * Because GMT_modules may be given files, memory locations, streams, etc., as input and output we
 * have to impose some structure as how this will work seamlessly.  Here, "GMT object" refers to
 * any of the 5 GMT resources: grids, images, datasets, textsets, and palettes.
 *
 * 1. When GMT allocates memory for a GMT object it sets its alloc_mode to GMT_ALLOC_INTERNALLY (1)
 *    and its alloc_level to <module level>.  This is 0 for the gmt.c UNIX application as well as
 *    for any external API (MEX, Python, Julia), 1 for any GMT module called, 2 for modules called
 *    by top-level modules, etc., as far down as the thread goes.
 * 2. Memory not allocated by GMT will have an implicit alloc_mode = GMT_ALLOC_EXTERNALLY [0]
 *    and alloc_level = 0 (i.e., gmt executable or API level) but it does not matter since such memory is
 *    only used for reading and we may never free it or reallocate it within GMT. This alloc_mode
 *    only applies to data arrays inside objects (e.g., G->data), not the GMT objects themselves.
 *    The GMT objects (the "containers") are freed at the end of their level, if not before.
 * 3. Memory passed into modules as "input file" requires no special treatment since its level
 *    will be lower than that of the module it is used in, and when that module tries to free it
 *    (directly with GMT_Destroy_Data or via end-of-module gmtapi_garbage_collection) it will skip
 *    it as its level does not match the current module level.  A module can only free memory that
 *    it allocated; the exception is the top-level gmt application.
 * 4. Passing memory out of a module (i.e., "writing to memory") requires that the calling function
 *    first create an output object and use the ID to encode the memory filename (@GMTAPI@-######).
 *    The object stores the level it was called at.  Pass the encoded filename as the output file.
 *    When GMT_Create_Data is called with no dimensions then the direction is set to GMT_OUT and
 *    we set the object's messenger flag to true.  This is used so that when the dataset we wish to
 *    return out of a module is built it replaces the empty initial dataset but inherits that dataset's
 *    alloc_level so it may survive the life of the module process.
 *    Internally, the memory that the module allocates (e.g., grid, dataset, etc.) will initially
 *    have an alloc_level matching the module level (and would be freed if written to a regular
 *    file).  However, when GMT_Write_Data is called and we branch into the GMT_REFERENCE case we
 *    instead take the following steps:
 *	a) The registered output API->object's data pointer is set to the GMT object that the
 *         module allocated (this is how we pass the data out of a module).
 *	b) The GMT object's alloc_level is changed to equal the output API->object's level (this
 *         is how it will survive beyond the end of the module).
 *	c) The API object originally pointing to the GMT object is flagged by having its variable
 *         no_longer_owner set to true (this is how we avoid freeing something twice). [PW: Why not just set it to NULL]
 *    When the module ends there are two API objects with references to the GMT object: the internal
 *    module object and the output object.  The first is set to NULL by gmtapi_garbage_collection because
 *    the object is no longer the owner of the data. The second is ignored because its level is too low.
 *    After that any empty API objects are removed (so the no_longer_owner one is removed), while
 *    the second survives the life of the module, as we require.
 *
 * Thus, at the session (gmt) level all GMT objects have alloc_level = 0 since anything higher will
 * have been freed by a module.  GMT_Destroy_Session finally calls gmtapi_garbage_collection a final
 * time and he frees any remaining GMT objects.
 *
 * Notes on family vs actual_family:
 * The S->actual_family contains the object type that we allocated.  However, we allow modules
 * that expect a DATASET to insted be passed a GMT_VECTOR or GMT_MATRIX.  If so then S->family
 * will be GMT_IS_DATASET while the actual_family remains GMT_VECTOR|GMT_MATRIX.  The i/o functions
 * GMT_Read_Data, GMT_Put_Record, etc knows how to deal with this.
 */

/*!
 * \file gmt_api.c
 * \brief Public functions for the GMT C/C++ API.
 */

#include "gmt_dev.h"
#include "gmt_internals.h"
#include "gmt_sharedlibs.h" 	/* Common shared libs structures */
#include <stdarg.h>
#include <curl/curl.h>

#ifdef HAVE_DIRENT_H_
#	include <dirent.h>
#endif

#ifdef HAVE_SYS_DIR_H_
#	include <sys/dir.h>
#endif

/* Possibly define missing shared library constants */
#ifndef DT_DIR
#	define DT_DIR 4
#endif

#ifndef RTLD_LAZY
#	define RTLD_LAZY 1
#endif

#ifdef WIN32	/* Special for Windows */
#	include <process.h>
#	define getpid _getpid
#endif

/* extern functions from various gmt_* only used here */
EXTERN_MSC void gmtfft_fourt_stats (struct GMT_CTRL *GMT, unsigned int n_columns, unsigned int n_rows, unsigned int *f, double *r, size_t *s, double *t);
EXTERN_MSC unsigned int gmtgrdio_free_grid_ptr (struct GMT_CTRL *GMT, struct GMT_GRID *G, bool free_grid);
EXTERN_MSC int gmtgrdio_init_grdheader (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *header, struct GMT_OPTION *options, uint64_t dim[], double wesn[], double inc[], unsigned int registration, unsigned int mode);

#define GMTAPI_MAX_ID 999999	/* Largest integer that will fit in the %06d format */
#define GMTAPI_UNLIMITED	0	/* Using 0 to mean we may allow 1 or more data objects of this family */

#ifdef FORTRAN_API
/* Global structure pointer needed for FORTRAN-77 [PW: not tested yet - is it even needed?] */
static struct GMTAPI_CTRL *GMT_FORTRAN = NULL;
#endif

static int GMTAPI_session_counter = 0;	/* Keeps track of the ID of new sessions for multi-session programs */

/*! Macros that report error, then return a NULL pointer, the error, or a value, respectively */
#define return_null(API,err) { gmtapi_report_error(API,err); return (NULL);}
#define return_error(API,err) { gmtapi_report_error(API,err); return (err);}
#define return_value(API,err,val) { gmtapi_report_error(API,err); return (val);}

/* We asked for subset of grid if the wesn pointer is not NULL and indicates a nonzero region */
#define full_region(wesn) (!wesn || (wesn[XLO] == wesn[XHI] && wesn[YLO] == wesn[YHI]))

/* DATASET and TEXTSET can be given via many individual files. */
#define multiple_files_ok(family) (family == GMT_IS_DATASET || family == GMT_IS_TEXTSET)

/* Misc. local text strings needed in this file only, used when debug verbose is on (-Vd).
 * NOTE: THe order of these depends on the order in the enums in gmt_resources.h! */

static const char *GMT_method[] = {"File", "Stream", "File Descriptor", "Memory Copy", "Memory Reference"};
static const char *GMT_family[] = {"Data Table", "GMT Grid", "GMT Image", "CPT", "GMT PostScript", "Text Table", "GMT Matrix", "GMT Vector", "GMT Coord"};
static const char *GMT_direction[] = {"Input", "Output"};
static const char *GMT_stream[] = {"Standard", "User-supplied"};
static const char *GMT_status[] = {"Unused", "In-use", "Used"};
static const char *GMT_geometry[] = {"Not Set", "Point", "Line", "Polygon", "Point|Line|Poly", "Line|Poly", "Surface", "Non-Geographical"};

/*! Two different i/o mode: GMT_Put|Get_Data vs GMT_Put|Get_Record */
enum GMT_enum_iomode {
	GMTAPI_BY_SET 	= 0,	/* Default is to read the entire dataset or texset */
	GMTAPI_BY_REC	= 1};	/* Means we will access the registere files on a record-by-record basis */

/*! Entries into dim[] for matrix or vector */
enum GMT_dim {
	GMTAPI_HDR_POS = 3,	/* Used with curr_pos to keep track of table headers only */
	GMTAPI_DIM_COL	= 0,	/* Holds the number of columns for vectors and x-nodes for matrix */
	GMTAPI_DIM_ROW = 1};	/* Holds the number of rows for vectors and y-nodes for matrix */

enum GMTAPI_enum_input {
	GMTAPI_OPTION_INPUT 	= 0,	/* Input resource specified via an option (e.g., -G<file>) */
	GMTAPI_MODULE_INPUT 	= 1};	/* Input resource specified via the module command line */

enum GMTAPI_enum_status {
	GMTAPI_GOT_SEGHEADER 	= -1,	/* Read a segment header */
	GMTAPI_GOT_SEGGAP 	= -2};	/* Detected a gap and insertion of new segment header */

enum GMTAPI_enum_via {
	GMT_VIA_VECTOR = 128,	/* Data passed via user matrix */
	GMT_VIA_MATRIX = 256};	/* Data passed via user vectors */

/*==================================================================================================
 *		PRIVATE FUNCTIONS ONLY USED BY THIS LIBRARY FILE
 *==================================================================================================
 *
 * api_* functions are static and only used in gmt_api.c
 * gmtapi_* functions are exported and may be used in other gmt_*.c files
 */

/* A few functions are declared here since it is used in so many places */
int gmtapi_report_error (void *V_API, int error);
int gmtapi_validate_id (struct GMTAPI_CTRL *API, int family, int object_ID, int direction, int module_input);
int gmtapi_unregister_io (struct GMTAPI_CTRL *API, int object_ID, unsigned int direction);
unsigned int gmtapi_count_objects (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int geometry, unsigned int direction, int *first_ID);
void gmtapi_close_grd (struct GMT_CTRL *GMT, struct GMT_GRID *G);
char *gmtapi_create_header_item (struct GMTAPI_CTRL *API, unsigned int mode, void *arg);

/* Series of one-line functions to assign val to a particular union member of array u at position row, rounding if integer output */
GMT_LOCAL void api_put_val_double (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->f8[row]  =                  val; }
GMT_LOCAL void api_put_val_float  (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->f4[row]  = (float)          val; }
GMT_LOCAL void api_put_val_ulong  (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->ui8[row] = (uint64_t)lrint(val); }
GMT_LOCAL void api_put_val_long   (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->si8[row] =  (int64_t)lrint(val); }
GMT_LOCAL void api_put_val_uint   (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->ui4[row] = (uint32_t)lrint(val); }
GMT_LOCAL void api_put_val_int    (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->si4[row] =  (int32_t)lrint(val); }
GMT_LOCAL void api_put_val_ushort (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->ui2[row] = (uint16_t)lrint(val); }
GMT_LOCAL void api_put_val_short  (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->si2[row] =  (int16_t)lrint(val); }
GMT_LOCAL void api_put_val_uchar  (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->uc1[row] =  (uint8_t)lrint(val); }
GMT_LOCAL void api_put_val_char   (union GMT_UNIVECTOR *u, uint64_t row, double val) { u->sc1[row] =   (int8_t)lrint(val); }

GMT_LOCAL void api_get_val_double (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->f8[row]; }
GMT_LOCAL void api_get_val_float  (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->f4[row]; }
GMT_LOCAL void api_get_val_ulong  (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = (double)u->ui8[row]; }	/* Must cast/truncate since longs integer range exceed that of double */
GMT_LOCAL void api_get_val_long   (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = (double)u->si8[row]; }	/* Must cast/truncate since longs integer range exceed that of double */
GMT_LOCAL void api_get_val_uint   (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->ui4[row]; }
GMT_LOCAL void api_get_val_int    (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->si4[row]; }
GMT_LOCAL void api_get_val_ushort (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->ui2[row]; }
GMT_LOCAL void api_get_val_short  (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->si2[row]; }
GMT_LOCAL void api_get_val_uchar  (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->uc1[row]; }
GMT_LOCAL void api_get_val_char   (union GMT_UNIVECTOR *u, uint64_t row, double *val) { *val = u->sc1[row]; }

typedef void (*GMT_putfunction)(union GMT_UNIVECTOR *, uint64_t, double  );
typedef void (*GMT_getfunction)(union GMT_UNIVECTOR *, uint64_t, double *);

GMT_LOCAL inline GMT_putfunction api_select_put_function (struct GMTAPI_CTRL *API, unsigned int type) {
	switch (type) {	/* Use type to select the correct put function with which to place a value in the union */
		case GMT_DOUBLE:	return (api_put_val_double);	break;
		case GMT_FLOAT:		return (api_put_val_float);	break;
		case GMT_ULONG:		return (api_put_val_ulong);	break;
		case GMT_LONG:		return (api_put_val_long);	break;
		case GMT_UINT:		return (api_put_val_uint);	break;
		case GMT_INT:		return (api_put_val_int);	break;
		case GMT_USHORT:	return (api_put_val_ushort);	break;
		case GMT_SHORT:		return (api_put_val_short);	break;
		case GMT_UCHAR:		return (api_put_val_uchar);	break;
		case GMT_CHAR:		return (api_put_val_char);	break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Internal error in api_select_put_function: Passed bad type (%d), Must abort\n", type);
			GMT_exit (API->GMT, GMT_RUNTIME_ERROR); return NULL;
			break;
	}
}

GMT_LOCAL inline GMT_getfunction api_select_get_function (struct GMTAPI_CTRL *API, unsigned int type) {
	switch (type) {	/* Use type to select the correct get function with which to extract a value from the union */
		case GMT_DOUBLE:	return (api_get_val_double);	break;
		case GMT_FLOAT:		return (api_get_val_float);	break;
		case GMT_ULONG:		return (api_get_val_ulong);	break;
		case GMT_LONG:		return (api_get_val_long);	break;
		case GMT_UINT:		return (api_get_val_uint);	break;
		case GMT_INT:		return (api_get_val_int);	break;
		case GMT_USHORT:	return (api_get_val_ushort);	break;
		case GMT_SHORT:		return (api_get_val_short);	break;
		case GMT_UCHAR:		return (api_get_val_uchar);	break;
		case GMT_CHAR:		return (api_get_val_char);	break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Internal error in api_select_get_function: Passed bad type (%d), Must abort\n", type);
			GMT_exit (API->GMT, GMT_RUNTIME_ERROR); return NULL;
			break;
	}
}

/*! . */
GMT_LOCAL int api_get_item (struct GMTAPI_CTRL *API, unsigned int family, void *data) {
	unsigned int i;
	int item;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;

	API->error = GMT_NOERROR;
	for (i = 0, item = GMT_NOTSET; item == GMT_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;				/* Empty object */
		S_obj = API->object[i];
		if (!S_obj->data) continue;		/* Empty data */
		if (S_obj->family != (enum GMT_enum_family)family) {		/* Not the required data type; check for exceptions... */
			if (family == GMT_IS_DATASET && (S_obj->family == GMT_IS_VECTOR || S_obj->family == GMT_IS_MATRIX))
				S_obj->family = GMT_IS_DATASET;	/* Vectors or Matrix masquerading as dataset are valid. Change their family here. */
			else	/* We don't like your kind */
				continue;
		}
		if (S_obj->data == data) item = i;	/* Found the requested data */
	}
	if (item == GMT_NOTSET) { API->error = GMT_NOT_A_VALID_ID; return (GMT_NOTSET); }	/* No such data found */
	return (item);
}

/*! . */
GMT_LOCAL inline uint64_t api_n_cols_needed_for_gaps (struct GMT_CTRL *GMT, uint64_t n) {
	/* Return the actual items needed (which may be more than n if gap testing demands it) */
	if (GMT->common.g.active) return (MAX (n, GMT->common.g.n_col));	/* n or n_col (if larger) */
	return (n);	/* No gap checking, n it is */
}

/*! . */
GMT_LOCAL inline void api_update_prev_rec (struct GMT_CTRL *GMT, uint64_t n_use) {
	/* Update previous record before reading the new record, but only if needed */
	if (GMT->current.io.need_previous) gmt_M_memcpy (GMT->current.io.prev_rec, GMT->current.io.curr_rec, n_use, double);
}

/*! . */
GMT_LOCAL int api_alloc_vectors (struct GMT_CTRL *GMT, struct GMT_VECTOR *V, uint64_t n_alloc) {
	/* Allocate space for each column according to data type; V->n_rows is not touched */
	uint64_t col;
	int error;

	if (!V) return (GMT_PTR_IS_NULL);			/* Nothing to allocate to */
	if (V->n_columns == 0) return (GMT_PTR_IS_NULL);	/* No columns specified */
	if (n_alloc == 0) return (GMT_N_ROWS_NOT_SET);		/* No rows specified */
	if (!V->data) return (GMT_PTR_IS_NULL);			/* Array of columns have not been allocated */
	for (col = 0; col < V->n_columns; col++) {
		if ((error = gmtlib_alloc_univector (GMT, &V->data[col], V->type[col], n_alloc)) != GMT_OK) return (error);
	}
	V->alloc_mode = GMT_ALLOC_INTERNALLY;
	return (GMT_OK);
}

/*! . */
GMT_LOCAL int api_alloc_grid (struct GMT_CTRL *GMT, struct GMT_GRID *Grid) {
	/* Use information in Grid header to allocate the grid data.
	 * We assume gmtgrdio_init_grdheader has been called. */

	if (Grid->data) return (GMT_PTR_NOT_NULL);
	if (Grid->header->size == 0U) return (GMT_SIZE_IS_ZERO);
	if ((Grid->data = gmt_M_memory_aligned (GMT, NULL, Grid->header->size, float)) == NULL) return (GMT_MEMORY_ERROR);
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_alloc_grid_xy (struct GMTAPI_CTRL *API, struct GMT_GRID *Grid) {
	/* Use information in Grid header to allocate the grid x/y vectors.
	 * We assume gmtgrdio_init_grdheader has been called. */

	if (Grid->x || Grid->y) return (GMT_PTR_NOT_NULL);
	Grid->x = GMT_Get_Coord (API, GMT_IS_GRID, GMT_X, Grid);	/* Get array of x coordinates */
	Grid->y = GMT_Get_Coord (API, GMT_IS_GRID, GMT_Y, Grid);	/* Get array of y coordinates */
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_alloc_image (struct GMT_CTRL *GMT, uint64_t *dim, struct GMT_IMAGE *Image) {
	/* Use information in Image header to allocate the image data.
	 * We assume gmtgrdio_init_grdheader has been called.
	 * If dim given and is 2 or 4 then we have 1 or 3 bands plus alpha channel */

	if (Image->data) return (GMT_PTR_NOT_NULL);
	if (Image->header->size == 0U) return (GMT_SIZE_IS_ZERO);
	if ((Image->data = gmt_M_memory_aligned (GMT, NULL, Image->header->size * Image->header->n_bands, unsigned char)) == NULL) return (GMT_MEMORY_ERROR);
	if (dim && (dim[GMT_Z] == 2 || dim[GMT_Z] == 4)) {
		if ((Image->alpha = gmt_M_memory_aligned (GMT, NULL, Image->header->size, unsigned char)) == NULL) return (GMT_MEMORY_ERROR);
	}
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_alloc_image_xy (struct GMTAPI_CTRL *API, struct GMT_IMAGE *Image) {
	/* Use information in Grid header to allocate the image x,y vectors.
	 * We assume gmtgrdio_init_grdheader has been called. */

	if (Image->x || Image->y) return (GMT_PTR_NOT_NULL);
	Image->x = GMT_Get_Coord (API, GMT_IS_IMAGE, GMT_X, Image);	/* Get array of x coordinates */
	Image->y = GMT_Get_Coord (API, GMT_IS_IMAGE, GMT_Y, Image);	/* Get array of y coordinates */
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_print_func (FILE *fp, const char *message) {
	/* Just print this message to fp.  It is being used indirectly via
	 * API->print_func.  Purpose of this is to allow external APIs such
	 * as MATLAB (which cannot use printf) to reset API->print_func to
	 * mexPrintf or similar functions. Default is api_print_func. */

	fprintf (fp, "%s", message);
	return 0;
}

/*! . */
GMT_LOCAL unsigned int api_gmtry (unsigned int geometry) {
	/* Return index to text representation in GMT_geometry[] for debug messages */
	if (geometry == GMT_IS_POINT)   return 1;
	if (geometry == GMT_IS_LINE)    return 2;
	if (geometry == GMT_IS_POLY)    return 3;
	if (geometry == GMT_IS_PLP)     return 4;
	if ((geometry & GMT_IS_LINE) && (geometry & GMT_IS_POLY)) return 5;
	if (geometry == GMT_IS_SURFACE) return 6;
	if (geometry == GMT_IS_NONE)    return 7;
	return 0;
}
/* We also need to return the pointer to an object given a void * address of that pointer.
 * This needs to be done on a per data-type basis, e.g., to cast that void * to a struct GMT_GRID **
 * so we may return the value at that address: */

GMT_LOCAL inline struct GMTAPI_CTRL * api_get_api_ptr     (struct GMTAPI_CTRL *ptr)  {return (ptr);}
GMT_LOCAL inline struct GMT_PALETTE * api_get_cpt_ptr     (struct GMT_PALETTE **ptr) {return (*ptr);}
GMT_LOCAL inline struct GMT_DATASET * api_get_dataset_ptr (struct GMT_DATASET **ptr) {return (*ptr);}
GMT_LOCAL inline struct GMT_TEXTSET * api_get_textset_ptr (struct GMT_TEXTSET **ptr) {return (*ptr);}
GMT_LOCAL inline struct GMT_GRID    * api_get_grid_ptr    (struct GMT_GRID **ptr)    {return (*ptr);}
GMT_LOCAL inline struct GMT_POSTSCRIPT      * api_get_ps_ptr      (struct GMT_POSTSCRIPT **ptr)      {return (*ptr);}
GMT_LOCAL inline struct GMT_MATRIX  * api_get_matrix_ptr  (struct GMT_MATRIX **ptr)  {return (*ptr);}
GMT_LOCAL inline struct GMT_VECTOR  * api_get_vector_ptr  (struct GMT_VECTOR **ptr)  {return (*ptr);}
GMT_LOCAL inline double      	    * api_get_coord_ptr   (double **ptr)             {return (*ptr);}
GMT_LOCAL inline struct GMT_IMAGE   * api_get_image_ptr (struct GMT_IMAGE **ptr) {return (*ptr);}
/* Various inline functs to convert void pointer to specific type */
GMT_LOCAL inline struct GMT_GRID_ROWBYROW * api_get_rbr_ptr (struct GMT_GRID_ROWBYROW *ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_FFT_INFO * api_get_fftinfo_ptr (struct GMT_FFT_INFO *ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_FFT_WAVENUMBER * api_get_fftwave_ptr (struct GMT_FFT_WAVENUMBER *ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_FFT_WAVENUMBER ** api_get_fftwave_addr (struct GMT_FFT_WAVENUMBER **ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_GRID    * api_get_grid_data (struct GMT_GRID *ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_IMAGE   * api_get_image_data (struct GMT_IMAGE *ptr) {return (ptr);}
GMT_LOCAL inline struct GMT_DATASET * api_get_dataset_data (struct GMT_DATASET *ptr) {return (ptr);}

/*! If API is not set or do_not_exit is false then we call system exit, else we move along */
GMT_LOCAL inline void api_exit (struct GMTAPI_CTRL *API, int code) {
	if (API->do_not_exit == false)
		GMT_exit (API->GMT, code);
}

/*! api_return_address is a convenience function that, given type, calls the correct converter */
GMT_LOCAL void *api_return_address (void *data, unsigned int type) {
	void *p = NULL;
	switch (type) {
		case GMT_IS_GRID:	p = api_get_grid_ptr (data);	break;
		case GMT_IS_DATASET:	p = api_get_dataset_ptr (data);	break;
		case GMT_IS_TEXTSET:	p = api_get_textset_ptr (data);	break;
		case GMT_IS_PALETTE:	p = api_get_cpt_ptr (data);	break;
		case GMT_IS_POSTSCRIPT:	p = api_get_ps_ptr (data);	break;
		case GMT_IS_MATRIX:	p = api_get_matrix_ptr (data);	break;
		case GMT_IS_VECTOR:	p = api_get_vector_ptr (data);	break;
		case GMT_IS_COORD:	p = api_get_coord_ptr (data);	break;
		case GMT_IS_IMAGE:	p = api_get_image_ptr (data);	break;
	}
	return (p);
}

/*! . */
struct GMTAPI_CTRL * gmt_get_api_ptr (struct GMTAPI_CTRL *ptr) {
	/* Clean casting of void to API pointer at start of a module
 	 * If ptr is NULL we are in deep trouble...
	 */
	if (ptr == NULL) return_null (NULL, GMT_NOT_A_SESSION);
	return (ptr);
}

/*! api_alloc_object_array is a convenience function that, given type, allocates an array of the corresponding container */
GMT_LOCAL void *api_alloc_object_array (struct GMTAPI_CTRL *API, unsigned int n_items, unsigned int type) {
	void *p = NULL;
	switch (type) {
		case GMT_IS_GRID:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_GRID *);		break;
		case GMT_IS_DATASET:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_DATASET *);	break;
		case GMT_IS_TEXTSET:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_TEXTSET *);	break;
		case GMT_IS_PALETTE:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_PALETTE *);	break;
		case GMT_IS_POSTSCRIPT:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_POSTSCRIPT *);	break;
		case GMT_IS_IMAGE:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_IMAGE *);		break;
		case GMT_IS_MATRIX:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_MATRIX *);	break;
		case GMT_IS_VECTOR:	p = gmt_M_memory (API->GMT, NULL, n_items, struct GMT_VECTOR *);	break;
	}
	return (p);
}

/*! p_func_uint64_t is used as a pointer to functions that returns a uint64_t index */
typedef uint64_t (*p_func_uint64_t) (uint64_t row, uint64_t col, uint64_t dim);

#ifdef DEBUG
/*! Can be used to display API->object info wherever it is called as part of a debug operation */
GMT_LOCAL void api_list_objects (struct GMTAPI_CTRL *API, char *txt) {
	unsigned int item, ext;
	struct GMTAPI_DATA_OBJECT *S;
	char message[GMT_LEN256] = {""}, O, M;
	/* if (API->deep_debug == false) return; */
	if (!gmt_M_is_verbose (API->GMT, GMT_MSG_DEBUG)) return;
	snprintf (message, GMT_LEN256, "==> %d API Objects at end of %s\n", API->n_objects, txt);
	GMT_Message (API, GMT_TIME_NONE, message);
	if (API->n_objects == 0) return;
	GMT_Message (API, GMT_TIME_NONE, "-----------------------------------------------------------\n");
	snprintf (message, GMT_LEN256, "K.. ID RESOURCE.... DATA........ FAMILY.... DIR... S O M L\n");
	GMT_Message (API, GMT_TIME_NONE, message);
	GMT_Message (API, GMT_TIME_NONE, "-----------------------------------------------------------\n");
	for (item = 0; item < API->n_objects; item++) {
		if ((S = API->object[item]) == NULL) continue;
		O = (S->no_longer_owner) ? 'N' : 'Y';
		M = (S->messenger) ? 'Y' : 'N';
		ext = (S->alloc_mode == GMT_ALLOC_EXTERNALLY) ? '*' : ' ';
		snprintf (message, GMT_LEN256, "%c%2d %2d %12" PRIxS " %12" PRIxS " %10s %6s %d %c %c %d\n", ext, item, S->ID, (size_t)S->resource, (size_t)S->data,
			GMT_family[S->family], GMT_direction[S->direction], S->status, O, M, S->alloc_level);
		GMT_Message (API, GMT_TIME_NONE, message);
	}
	GMT_Message (API, GMT_TIME_NONE, "-----------------------------------------------------------\n");
}

/*! Mostly for debugging */
GMT_LOCAL void api_set_object (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *obj) {
	/* This is mostly for debugging and may go away or remain under DEBUG */
	GMT_Report (API, GMT_MSG_DEBUG, "Set_Object for family: %d\n", obj->family);
	switch (obj->family) {
		case GMT_IS_GRID:	obj->G = obj->data; break;
		case GMT_IS_DATASET:	obj->D = obj->data; break;
		case GMT_IS_TEXTSET:	obj->T = obj->data; break;
		case GMT_IS_PALETTE:	obj->C = obj->data; break;
		case GMT_IS_POSTSCRIPT:		obj->P = obj->data; break;
		case GMT_IS_MATRIX:	obj->M = obj->data; break;
		case GMT_IS_VECTOR:	obj->V = obj->data; break;
		case GMT_IS_COORD:	break;	/* No worries */
		case GMT_IS_IMAGE:	obj->I = obj->data; break;
		case GMT_N_FAMILIES:	break;
	}
}
#endif

/*! . */
GMT_LOCAL int api_get_ppid (struct GMTAPI_CTRL *API) {
	/* Return the parent process ID [i.e., shell for command line use or gmt app for API] */
	int ppid = -1;
	gmt_M_unused(API);
#if defined(WIN32) || defined(DEBUG_MODERN)
	/* OK, the trouble is the following. On Win if the executables are run from within MSYS
	   api_get_ppid returns different values for each call, and this completely breaks the idea
	   using the PPID (parent PID) to create unique file names.
	   So, given that we didn't yet find a way to make this work from within MSYS (and likely Cygwin)
	   we are forcing PPID = 0 in all Windows variants. */
	ppid = 0;
#else
	if (API->external)	/* Return ID of the gmt application instead for external interfaces */
		ppid = getpid ();
	else	/* Here we are probably running from the command line and want the shell's PID */
		ppid = getppid(); /* parent process id */
#endif
	return (ppid);
}

#if 0
/* This was our effort to get PPID under Windows.  Remains as comments for now */
#ifdef _WIN32
	int pid = GetCurrentProcessId ();
	HANDLE h = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof (PROCESSENTRY32);

	if (Process32First(h, &pe)) {
		do {
			if (pe.th32ProcessID == pid)
				ppid = pe.th32ParentProcessID;
		} while (ppid == -1 && Process32Next(h, &pe));
	}
	CloseHandle (h);
#endif
#endif

/*! . */
GMT_LOCAL char *api_lib_tag (char *name) {
	/* Pull out the tag from a name like <tag>[.extension] */
	char *extension = NULL, *pch = NULL, *tag = NULL;
	if (!strchr (name, '.')) return NULL;	/* No file with extension given, probably just a directory due to user confusion */
	tag = strdup (name);
	extension = strrchr (tag, '.'); /* last period in name */
	*extension = '\0'; /* remove extension */
	/* if name has the "_w32|64" suffix or any other suffix that starts with a '_', remove it. */
	pch = strrchr(tag, '_');
	if (pch) *pch = '\0';
	return (tag);
}

/*! . */
GMT_LOCAL int api_init_sharedlibs (struct GMTAPI_CTRL *API) {
	/* At the end of GMT_Create_Session we are done with processing gmt.conf.
	 * We can now determine how many shared libraries and plugins to consider, and open the core lib */
	struct GMT_CTRL *GMT = API->GMT;
	unsigned int n_custom_libs = 0, k, e, n_alloc = GMT_TINY_CHUNK;
	char text[GMT_LEN256] = {""}, plugindir[GMT_LEN256] = {""}, path[GMT_LEN256] = {""};
	char *libname = NULL, **list = NULL;
#ifdef WIN32
	char *extension[1] = {".dll"};
	unsigned int n_extensions = 1;
#elif  defined(__APPLE__)	/* Look for both .so and .dylib shared libs on OSX */
	char *extension[2] = {".so", ".dylib"};
	unsigned int n_extensions = 2;
#else	/* Linux, etc. only use .so */
	char *extension[1] = {".so"};
	unsigned int n_extensions = 1;
#endif

#ifdef SUPPORT_EXEC_IN_BINARY_DIR
	/* If SUPPORT_EXEC_IN_BINARY_DIR is defined we try to load plugins from the
	 * build tree */

	/* Only true, when we are running in a subdir of GMT_BINARY_DIR_SRC_DEBUG: */
	bool running_in_bindir_src = !strncmp (GMT->init.runtime_bindir, GMT_BINARY_DIR_SRC_DEBUG, strlen(GMT_BINARY_DIR_SRC_DEBUG));
#endif

	API->lib = gmt_M_memory (GMT, NULL, n_alloc, struct Gmt_libinfo);

	/* 1. Load the GMT core library by default [unless static build] */
	/* Note: To extract symbols from the currently executing process we need to load it as a special library.
	 * This is done by passing NULL under Linux and by calling GetModuleHandleEx under Windows, hence we
	 * use the dlopen_special call which is defined in gmt_sharedlibs.c */

	API->lib[0].name = strdup ("core");
	API->lib[0].path = strdup (GMT_CORE_LIB_NAME);
	GMT_Report (API, GMT_MSG_DEBUG, "Shared Library # 0 (core). Path = %s\n", API->lib[0].path);
	++n_custom_libs;
#ifdef BUILD_SHARED_LIBS
	GMT_Report (API, GMT_MSG_DEBUG, "Loading core GMT shared library: %s\n", API->lib[0].path);
	if ((API->lib[0].handle = dlopen_special (API->lib[0].path)) == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error loading core GMT shared library: %s\n", dlerror());
		api_exit (API, GMT_RUNTIME_ERROR); return GMT_RUNTIME_ERROR;
	}
	dlerror (); /* Clear any existing error */
#endif

	/* 3. Add any plugins installed in <installdir>/lib/gmt/plugins */

	if (GMT->init.runtime_libdir) {	/* Successfully determined runtime dir for shared libs */
#ifdef SUPPORT_EXEC_IN_BINARY_DIR
		if ( running_in_bindir_src && access (GMT_BINARY_DIR_SRC_DEBUG "/plugins", R_OK|X_OK) == 0 ) {
			/* Running in build dir: search plugins in build-dir/src/plugins */
			strncat (plugindir, GMT_BINARY_DIR_SRC_DEBUG "/plugins", GMT_LEN256-1);
#ifdef XCODER
			strcat (plugindir, "/Debug");	/* The Xcode plugin path for Debug */
#endif
		}
		else
#endif
		{
		/* Set full path to the core library */
		snprintf (plugindir, GMT_LEN256, "%s/%s", GMT->init.runtime_libdir, GMT_CORE_LIB_NAME);
		if (!GMT->init.runtime_library) GMT->init.runtime_library = strdup (plugindir);

#ifdef WIN32
			snprintf (plugindir, GMT_LEN256, "%s/gmt_plugins", GMT->init.runtime_libdir);	/* Generate the Win standard plugins path */
#else
			snprintf (plugindir, GMT_LEN256, "%s/gmt" GMT_INSTALL_NAME_SUFFIX "/plugins", GMT->init.runtime_libdir);	/* Generate the *nix standard plugins path */
#endif
		}
		if (!GMT->init.runtime_plugindir) GMT->init.runtime_plugindir = strdup (plugindir);
		GMT_Report (API, GMT_MSG_DEBUG, "Loading GMT plugins from: %s\n", plugindir);
		for (e = 0; e < n_extensions; e++) {	/* Handle case of more than one allowed shared library extension */
			if ((list = gmtlib_get_dir_list (GMT, plugindir, extension[e]))) {	/* Add these files to the libs */
				k = 0;
				while (list[k]) {
					snprintf (path, GMT_LEN256, "%s/%s", plugindir, list[k]);
					if (access (path, R_OK))
						GMT_Report (API, GMT_MSG_NORMAL, "Shared Library %s cannot be found or read!\n", path);
					else {
						API->lib[n_custom_libs].name = api_lib_tag (list[k]);
						API->lib[n_custom_libs].path = strdup (path);
						GMT_Report (API, GMT_MSG_DEBUG, "Shared Library # %d (%s). Path = %s\n", n_custom_libs, API->lib[n_custom_libs].name, API->lib[n_custom_libs].path);
						n_custom_libs++;			/* Add up entries found */
						if (n_custom_libs == n_alloc) {		/* Allocate more memory for list */
							n_alloc <<= 1;
							API->lib = gmt_M_memory (GMT, API->lib, n_alloc, struct Gmt_libinfo);
						}
					}
					++k;
				}
				gmtlib_free_dir_list (GMT, &list);
			}
		}
	}

	/* 4. Add any custom GMT libraries to the list of libraries/plugins to consider, if specified.
	      We will find when trying to open if any of these are actually available. */

	if (GMT->session.CUSTOM_LIBS) {	/* We specified custom shared libraries */
		k = (unsigned int)strlen (GMT->session.CUSTOM_LIBS) - 1;	/* Index of last char in CUSTOM_LIBS */
		if (GMT->session.CUSTOM_LIBS[k] == '/' || GMT->session.CUSTOM_LIBS[k] == '\\') {	/* We gave CUSTOM_LIBS as a subdirectory, add all files found inside it to shared libs list */
			strcpy (plugindir, GMT->session.CUSTOM_LIBS);
			plugindir[k] = '\0';	/* Chop off trailing slash */
			GMT_Report (API, GMT_MSG_DEBUG, "Loading custom GMT plugins from: %s\n", plugindir);
			for (e = 0; e < n_extensions; e++) {
				if ((list = gmtlib_get_dir_list (GMT, plugindir, extension[e]))) {	/* Add these to the libs */
					k = 0;
					while (list[k]) {
						snprintf (path, GMT_LEN256, "%s/%s", plugindir, list[k]);
						if (access (path, R_OK)) {
							GMT_Report (API, GMT_MSG_NORMAL, "Shared Library %s cannot be found or read!\n", path);
							GMT_Report (API, GMT_MSG_NORMAL, "Check that your GMT_CUSTOM_LIBS (in gmt.conf, perhaps) is correct\n");
						}
						else if ((API->lib[n_custom_libs].name = api_lib_tag (list[k]))) {
							API->lib[n_custom_libs].path = strdup (path);
							GMT_Report (API, GMT_MSG_DEBUG, "Shared Library # %d (%s). Path = \n", n_custom_libs, API->lib[n_custom_libs].name, API->lib[n_custom_libs].path);
							n_custom_libs++;		/* Add up entries found */
							if (n_custom_libs == n_alloc) {	/* Allocate more memory for list */
								n_alloc <<= 1;
								API->lib = gmt_M_memory (GMT, API->lib, n_alloc, struct Gmt_libinfo);
							}
						}
						else
							GMT_Report (API, GMT_MSG_NORMAL, "Shared Library %s has no extension! Ignored\n", list[k]);
						++k;
					}
					gmtlib_free_dir_list (GMT, &list);
				}
			}
		}
		else {	/* Just a list with one or more comma-separated library paths */
			unsigned int pos = 0;
			while (gmt_strtok (GMT->session.CUSTOM_LIBS, ",", &pos, text)) {
				libname = strdup (basename (text));		/* Last component from the pathname */
				if (access (text, R_OK)) {
					GMT_Report (API, GMT_MSG_NORMAL, "Shared Library %s cannot be found or read!\n", text);
					GMT_Report (API, GMT_MSG_NORMAL, "Check that your GMT_CUSTOM_LIBS (in gmt.conf, perhaps) is correct\n");
				}
				else if ((API->lib[n_custom_libs].name = api_lib_tag (libname))) {
					API->lib[n_custom_libs].path = strdup (text);
					GMT_Report (API, GMT_MSG_DEBUG, "Shared Library # %d (%s). Path = \n", n_custom_libs, API->lib[n_custom_libs].name, API->lib[n_custom_libs].path);
					n_custom_libs++;		/* Add up entries found */
					if (n_custom_libs == n_alloc) {	/* Allocate more memory for list */
						n_alloc <<= 1;
						API->lib = gmt_M_memory (GMT, API->lib, n_alloc, struct Gmt_libinfo);
					}
				}
				else
					GMT_Report (API, GMT_MSG_NORMAL, "Shared Library %s has no extension! Ignored\n", text);
				gmt_M_str_free (libname);
			}
		}
	}

	API->n_shared_libs = n_custom_libs;	/* Update total number of shared libraries */
	API->lib = gmt_M_memory (GMT, API->lib, API->n_shared_libs, struct Gmt_libinfo);

	return (GMT_NOERROR);
}

/*! Free items in the shared lib list */
GMT_LOCAL void api_free_sharedlibs (struct GMTAPI_CTRL *API) {
	unsigned int k;
	for (k = 0; k < API->n_shared_libs; k++) {
		if (k > 0 && API->lib[k].handle && dlclose (API->lib[k].handle))
			GMT_Report (API, GMT_MSG_NORMAL, "Error closing GMT %s shared library: %s\n", API->lib[k].name, dlerror());
		gmt_M_str_free (API->lib[k].name);
		gmt_M_str_free (API->lib[k].path);
	}
	gmt_M_free (API->GMT, API->lib);
	API->n_shared_libs = 0;
}

/* Copy a file from the GMT auto-download directory or the Internet.  We recognize
 * different types and names of files.
 * 1. There are data sets of use to all GMT users, such as global relief:
 * 	   earth_relief_<res>.grd	Various global relief grids
 * We may add more data later but this is our start.
 * 2. Data sets only used to run an example or a test script
 * and these are all called @*, i.e., a '@' is pre-pended to the name.
 * They live in a cache subdirectory under the GMT_DATA_URL
 * and will be placed in a cache directory in the users ~/.gmt directory.
 * 3. Generic URLs starting with http:, https:, or ftp:  These will be
 * downloaded to the cache directory.
 * If auto-download is enabled and a requested input file matches these
 * names and not found by normal means then we download the file to the
 * user-directory.  All places that open files (GMT_Read_Data) will do
 * this check by first calling gmt_download_file_if_not_found.
 */

struct FtpFile {
	const char *filename;
	FILE *fp;
};

static size_t fwrite_callback (void *buffer, size_t size, size_t nmemb, void *stream) {
	struct FtpFile *out = (struct FtpFile *)stream;
	if (out && !out->fp) { /* open file for writing */
		out->fp = fopen (out->filename, "wb");
		if (!out->fp)
			return -1; /* failure, can't open file to write */
	}
	return fwrite (buffer, size, nmemb, out->fp);
}

unsigned int gmt_download_file_if_not_found (struct GMT_CTRL *GMT, const char* file_name) {
	/* Downloads a file if not found locally.  Returns the position in file_name of the
 	 * start of the actual file (e.g., if given an URL). */
	unsigned int kind = 0, dir = 0, pos = 0;
	int curl_err = 0;
	CURL *Curl = NULL;
	static char *ftp_dir[2] = {"/cache", ""}, *name[2] = {"CACHE", "USER"};
	char *user_dir[2] = {GMT->session.CACHEDIR, GMT->session.USERDIR};
	char url[PATH_MAX] = {""}, local_path[PATH_MAX] = {""}, *c = NULL, *file = strdup (file_name);
	struct FtpFile ftpfile = {NULL, NULL};

	/* Because file_name may be <file>, @<file>, or URL/<file> we must find start of <file> */
	if (gmt_M_file_is_cache (file)) {	/* A leading '@' was found */
		pos = 1;
		if ((c = strchr (file, '?')))	/* Netcdf directive since URL was handled above */
			c[0] = '\0';
		else if ((c = strchr (file, '=')))	/* Grid attributes */
			c[0] = '\0';
	}
	else if (gmt_M_file_is_url (file)) {	/* A remote file given via an URL */
		pos = gmtlib_get_pos_of_filename (file);	/* Start of file in URL (> 0) */
		if ((c = strchr (file, '?')) && !strchr (file, '='))	/* Must be a netCDF sliced URL file so chop off the layer/variable specifications */
			c[0] = '\0';
		else if ((c = strchr (file, '=')))	/* Grid attributes */
			c[0] = '\0';
	}
	else if ((c = strchr (file, '?')))	/* Netcdf directive since URLs and caches were handled above */
		c[0] = '\0';	/* and pos = 0 */

	/* Return immediately if cannot be downloaded (for various reasons) */
	if (!gmtlib_file_is_downloadable (GMT, file, &kind)) {
		gmt_M_str_free (file);
		return (pos);
	}
	dir = (kind == GMT_DATA_FILE) ? GMT_DATA_DIR : GMT_CACHE_DIR;	/* Only GMT datasets should go data dir; all else in cache */
	if (user_dir[dir] == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "The GMT_%s directory is not defined - download file to current directory\n", name[dir]);
		sprintf (local_path, "%s", &file[pos]);
	}
	/* Here we will try to download a file */

  	if ((Curl = curl_easy_init ()) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to initiate curl - cannot obtain %s\n", &file[pos]);
		gmt_M_str_free (file);
		return 0;
	}
	if (curl_easy_setopt (Curl, CURLOPT_SSL_VERIFYPEER, 0L)) {		/* Tell libcurl to not verify the peer */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to set curl option to not verify the peer\n");
		gmt_M_str_free (file);
		return 0;
	}
	if (curl_easy_setopt (Curl, CURLOPT_FOLLOWLOCATION, 1L)) {		/* Tell libcurl to follow 30x redirects */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to set curl option to follow redirects\n");
		gmt_M_str_free (file);
		return 0;
	}
	if (user_dir[dir]) sprintf (local_path, "%s/%s", user_dir[dir], &file[pos]);
	if (kind == GMT_URL_QUERY) {	/* Cannot have ?para=value etc in filename */
		c = strchr (local_path, '?');
		if (c) c[0] = '\0';	/* Chop off ?CGI parameters from local_path */
	}
	if (kind == GMT_URL_FILE || kind == GMT_URL_QUERY)	/* General URL given */
		sprintf (url, "%s", file);
	else			/* Use GMT ftp dir, possible from subfolder cache */
		sprintf (url, "%s%s/%s", GMT_DATA_URL, ftp_dir[dir], &file[pos]);

 	if (curl_easy_setopt (Curl, CURLOPT_URL, url)) {	/* Set the URL to copy */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to set curl option to read from %s\n", url);
		gmt_M_str_free (file);
		return 0;
	}
	ftpfile.filename = local_path;	/* Set pointer to local filename */
	/* Define our callback to get called when there's data to be written */
	if (curl_easy_setopt (Curl, CURLOPT_WRITEFUNCTION, fwrite_callback)) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to set curl output callback function\n");
		gmt_M_str_free (file);
		return 0;
	}
	/* Set a pointer to our struct to pass to the callback */
	if (curl_easy_setopt (Curl, CURLOPT_WRITEDATA, &ftpfile)) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Failed to set curl option to write to %s\n", local_path);
		gmt_M_str_free (file);
		return 0;
	}
	GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Downloading file %s ...\n", url);
	if ((curl_err = curl_easy_perform (Curl))) {	/* Failed, give error message */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Libcurl Error: %s\n", curl_easy_strerror (curl_err));
		fclose (ftpfile.fp);
		ftpfile.fp = NULL;
		if (gmt_remove_file (GMT, local_path))
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Could not even remove file %s\n", local_path);
	}
	curl_easy_cleanup (Curl);
	if (ftpfile.fp) /* close the local file */
		fclose (ftpfile.fp);

	if (kind == GMT_URL_QUERY) {	/* Cannot have ?para=value etc in local filename */
		c = strchr (file_name, '?');
		if (c) c[0] = '\0';	/* Chop off ?CGI parameters from local_path */
	}
	if (gmt_M_is_verbose (GMT, GMT_MSG_VERBOSE)) {	/* Say a few things about the file we got */
		struct stat buf;
		if (stat (local_path, &buf))
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error: Could not determine size of downloaded file %s\n", &file_name[pos]);
		else
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Download complete [Got %s].\n", gmt_memory_use (buf.st_size, 3));
	}
	gmt_M_str_free (file);

	return (pos);
}

/* The basic gmtread|write module meat; used by external APIs only, such as the GMT/MATLAB API */

/*! Duplicate ifile on ofile.  Calling program is responsible to ensure correct args are passed */
int gmt_copy (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int direction, char *ifile, char *ofile) {
	double *wesn = NULL;	/* For grid and image subsets */
	struct GMT_DATASET *D = NULL;
	struct GMT_TEXTSET *T = NULL;
	struct GMT_PALETTE *C = NULL;
	struct GMT_GRID *G = NULL;
	struct GMT_POSTSCRIPT *P = NULL;
	struct GMT_IMAGE *I = NULL;
	struct GMT_MATRIX *M = NULL;
	struct GMT_VECTOR *V = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	API->error = GMT_NOERROR;
	GMT_Report (API, GMT_MSG_VERBOSE, "Read %s from %s and write to %s\n", GMT_family[family], ifile, ofile);
	GMT = API->GMT;

	switch (family) {
		case GMT_IS_DATASET:
			if ((D = GMT_Read_Data (API, GMT_IS_DATASET, GMT_IS_FILE, GMT_IS_POINT, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_DATASET, GMT_IS_FILE, D->geometry, D->io_mode | GMT_IO_RESET, NULL, ofile, D) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_TEXTSET:
			if ((T = GMT_Read_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_NONE, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_TEXTSET, GMT_IS_FILE, GMT_IS_NONE, T->io_mode | GMT_IO_RESET, NULL, ofile, T) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_GRID:
			wesn = (direction == GMT_IN && GMT->common.R.active[RSET]) ? GMT->common.R.wesn : NULL;
			if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_READ_NORMAL, wesn, ifile, NULL)) == NULL)
				return (API->error);
			wesn = (direction == GMT_OUT && GMT->common.R.active[RSET]) ? GMT->common.R.wesn : NULL;
			if (GMT_Write_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_CONTAINER_AND_DATA | GMT_IO_RESET, wesn, ofile, G) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_IMAGE:
			wesn = (direction == GMT_IN && GMT->common.R.active[RSET]) ? GMT->common.R.wesn : NULL;
			if ((I = GMT_Read_Data (API, GMT_IS_IMAGE, GMT_IS_FILE, GMT_IS_SURFACE, GMT_READ_NORMAL, wesn, ifile, NULL)) == NULL)
				return (API->error);
			wesn = (direction == GMT_OUT && GMT->common.R.active[RSET]) ? GMT->common.R.wesn : NULL;
			if (GMT_Write_Data (API, GMT_IS_IMAGE, GMT_IS_FILE, GMT_IS_SURFACE, GMT_CONTAINER_AND_DATA | GMT_IO_RESET, wesn, ofile, I) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_PALETTE:
			if ((C = GMT_Read_Data (API, GMT_IS_PALETTE, GMT_IS_FILE, GMT_IS_NONE, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_PALETTE, GMT_IS_FILE, GMT_IS_NONE, C->mode | GMT_IO_RESET, NULL, ofile, C) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_POSTSCRIPT:
			if ((P = GMT_Read_Data (API, GMT_IS_POSTSCRIPT, GMT_IS_FILE, GMT_IS_NONE, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_POSTSCRIPT, GMT_IS_FILE, GMT_IS_NONE, GMT_IO_RESET, NULL, ofile, P) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_MATRIX:
			if ((M = GMT_Read_Data (API, GMT_IS_MATRIX, GMT_IS_FILE, GMT_IS_NONE, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_MATRIX, GMT_IS_FILE, GMT_IS_NONE, GMT_IO_RESET, NULL, ofile, M) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_VECTOR:
			if ((V = GMT_Read_Data (API, GMT_IS_VECTOR, GMT_IS_FILE, GMT_IS_NONE, GMT_READ_NORMAL, NULL, ifile, NULL)) == NULL)
				return (API->error);
			if (GMT_Write_Data (API, GMT_IS_VECTOR, GMT_IS_FILE, GMT_IS_NONE, GMT_IO_RESET, NULL, ofile, V) != GMT_OK)
				return (API->error);
			break;
		case GMT_IS_COORD:
			GMT_Report (API, GMT_MSG_VERBOSE, "No external read or write support yet for object %s\n", GMT_family[family]);
			return_error(API, GMT_NOT_A_VALID_FAMILY);
			break;
		default:
			GMT_Report (API, GMT_MSG_VERBOSE, "Internal error, family = %d\n", family);
			return_error(API, GMT_NOT_A_VALID_FAMILY);
			break;
	}

	return (API->error);
}

/* Note: Many/all of these do not need to check if API == NULL since they are called from functions that do. */
/* Private functions used by this library only.  These are not accessed outside this file. */

GMT_LOCAL unsigned int api_pick_out_col_number (struct GMT_CTRL *GMT, unsigned int col) {
	/* Return the next column to be reported on output */
	unsigned int col_pos;
	if (GMT->common.o.active)	/* -o has selected some columns */
		col_pos = GMT->current.io.col[GMT_OUT][col].col;	/* Which data column to pick */
	else if (GMT->current.setting.io_lonlat_toggle[GMT_OUT] && col < GMT_Z)	/* Worry about -: for lon,lat */
		col_pos = 1 - col;	/* Write lat/lon instead of lon/lat */
	else
		col_pos = col;	/* Just goto that column */
	return (col_pos);
}

/*! . */
GMT_LOCAL double api_select_dataset_value (struct GMT_CTRL *GMT, struct GMT_DATASEGMENT *S, unsigned int row, unsigned int col) {
	/* For binary output of a data table segment via external matrix, we must select correct col entry and possibly make adjustments */
	double val;
	unsigned int col_pos = api_pick_out_col_number (GMT, col);
	val = (col_pos >= S->n_columns) ? GMT->session.d_NaN : S->data[col_pos][row];	/* If we request a column beyond length of array, return NaN */
	if (GMT->common.d.active[GMT_OUT] && gmt_M_is_dnan (val)) val = GMT->common.d.nan_proxy[GMT_OUT];	/* Write this value instead of NaNs */
	if (GMT->current.io.col_type[GMT_OUT][col_pos] == GMT_IS_LON) gmt_lon_range_adjust (GMT->current.io.geo.range, &val);	/* Set longitude range */
	return (val);
}

/*! . */
GMT_LOCAL double api_select_record_value (struct GMT_CTRL *GMT, double *record, unsigned int col, unsigned int n_colums) {
	/* For binary output of data record via external matrix, we must select correct col entry and possibly make adjustments */
	double val;
	unsigned int col_pos = api_pick_out_col_number (GMT, col);
	val = (col_pos >= n_colums) ? GMT->session.d_NaN : record[col_pos];	/* If we request a column beyond length of array, return NaN */
	if (GMT->common.d.active[GMT_OUT] && gmt_M_is_dnan (val)) val = GMT->common.d.nan_proxy[GMT_OUT];	/* Write this value instead of NaNs */
	if (GMT->current.io.col_type[GMT_OUT][col_pos] == GMT_IS_LON) gmt_lon_range_adjust (GMT->current.io.geo.range, &val);	/* Set longitude range */
	return (val);
}

GMT_LOCAL unsigned int api_pick_in_col_number (struct GMT_CTRL *GMT, unsigned int col) {
	/* Return the next column to be selected on input */
	unsigned int col_pos;
	if (GMT->common.i.active)	/* -i has selected some columns */
		col_pos = GMT->current.io.col[GMT_IN][col].col;	/* Which data column to pick */
#if 0
	else if (GMT->current.setting.io_lonlat_toggle[GMT_IN] && col < GMT_Z)	/* Worry about -: for lon,lat */
		col_pos = 1 - col;	/* Read lat/lon instead of lon/lat */
#endif
	else
		col_pos = col;	/* Just goto that column */
	return (col_pos);
}

/*! . */
GMT_LOCAL double api_get_record_value (struct GMT_CTRL *GMT, double *record, uint64_t col, uint64_t n_colums) {
	/* For binary input of data record via external matrix, we must select correct col entry and possibly make adjustments */
	double val;
	unsigned int col_pos;
	col_pos = api_pick_in_col_number (GMT, (unsigned int)col);
	val = (col_pos >= n_colums) ? GMT->session.d_NaN : record[col_pos];	/* If we request a column beyond length of array, return NaN */
	if (GMT->common.d.active[GMT_IN] && gmt_M_is_dnan (val)) val = GMT->common.d.nan_proxy[GMT_IN];	/* Write this value instead of NaNs */
	if (GMT->current.io.col_type[GMT_IN][col_pos] == GMT_IS_LON) gmt_lon_range_adjust (GMT->current.io.geo.range, &val);	/* Set longitude range */
	return (val);
}

/*! . */
GMT_LOCAL int api_bin_input_memory (struct GMT_CTRL *GMT, uint64_t n, uint64_t n_use) {
	/* Read function which gets one record from the memory reference.
 	 * The current data record has already been read from wherever and is available in GMT->current.io.curr_rec */
	unsigned int status;
	gmt_M_unused(n);

	GMT->current.io.status = GMT_IO_DATA_RECORD;	/* Default status we expect, but this may change below */
	GMT->current.io.rec_no++;			/* One more input record read */
	status = gmtlib_process_binary_input (GMT, n_use);	/* Check for segment headers */
	if (status == 1) return (GMTAPI_GOT_SEGHEADER);	/* A segment header was found and we are done here */
	if (gmtlib_gap_detected (GMT)) { gmtlib_set_gap (GMT); return (GMTAPI_GOT_SEGGAP); }	/* Gap forced a segment header to be issued and we get out */
	GMT->current.io.pt_no++;	/* Actually got a valid data record */
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL char * api_tictoc_string (struct GMTAPI_CTRL *API, unsigned int mode) {
	/* Optionally craft a leading timestamp.
	 * mode = 0:	No time stamp
	 * mode = 1:	Abs time stamp formatted via GMT_TIME_STAMP
	 * mode = 2:	Report elapsed time since last reset.
	 * mode = 4:	Reset elapsed time to 0, no time stamp.
	 * mode = 6:	Reset elapsed time and report it as well.
	 */
	time_t right_now;
	clock_t toc = 0;
	unsigned int H, M, S, milli;
	double T;
	static char stamp[GMT_LEN256] = {""};

	if (mode == 0) return NULL;		/* no timestamp requested */
	if (mode > 1) toc = clock ();		/* Elapsed time requested */
	if (mode & 4) API->GMT->current.time.tic = toc;	/* Reset previous timestamp to now */

	switch (mode) {	/* Form output timestamp string */
		case 1:	/* Absolute time stamp */
			right_now = time ((time_t *)0);
			strftime (stamp, sizeof(stamp), API->GMT->current.setting.format_time_stamp, localtime (&right_now));
			break;
		case 2:	/* Elapsed time stamp */
		case 6:
			T = (double)(toc - (clock_t)API->GMT->current.time.tic);	/* Elapsed time in ticks */
			T /= CLOCKS_PER_SEC;	/* Elapsed time in seconds */
			H = urint (floor (T * GMT_SEC2HR));
			T -= H * GMT_HR2SEC_I;
			M = urint (floor (T * GMT_SEC2MIN));
			T -= M * GMT_MIN2SEC_I;
			S = urint (floor (T));
			T -= S;
			milli = urint (T*1000.0);	/* Residual milli-seconds */
			snprintf (stamp, GMT_LEN256, "Elapsed time %2.2u:%2.2u:%2.2u.%3.3u", H, M, S, milli);
			break;
		default: break;
	}
	return (stamp);
}

/*! . */
GMT_LOCAL unsigned int api_add_existing (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int geometry, unsigned int direction, int *first_ID) {
	/* In this mode, we find all registrered resources of matching family,geometry,direction that are unused and turn variable selected to true. */
	unsigned int i, n;

	*first_ID = GMT_NOTSET;	/* Not found yet */
	for (i = n = 0; i < API->n_objects; i++) {
		if (!API->object[i]) continue;	/* A freed object, skip */
		if (API->object[i]->direction != (enum GMT_enum_std)direction) continue; /* Wrong direction */
		if (API->object[i]->geometry  != (enum GMT_enum_geometry)geometry) continue;  /* Wrong geometry */
		if (API->object[i]->status    != GMT_IS_UNUSED) continue;  /* Already used */
		if (family != API->object[i]->family) continue;		   /* Wrong data type */
		n++;	/* Found one that satisfied requirements */
		if (*first_ID == GMT_NOTSET) *first_ID = API->object[i]->ID;	/* Get the ID of the first that passed the test */
		API->object[i]->selected = true;	/* Make this an active object for the coming i/o operation */
	}
	return (n);
}

/* These functions are support functions for the API function GMT_Encode_Options:
 *	api_key_to_family
 *	api_process_keys
 *	api_get_key
 *	api_found_marker
 *
 * The "keys" refer to the contents of the THIS_MODULE_KEYS set in each module.
 */

/* Indices into the keys triple codes */
#define K_OPT			0
#define K_FAMILY		1
#define K_DIR			2
#define K_EQUAL			3
#define K_MODIFIER		4
#define GMT_FILE_NONE		0
#define GMT_FILE_EXPLICIT	1
#define GMT_FILE_IMPLICIT	2

#define K_PRIMARY			0
#define K_SECONDARY			1

#define API_PRIMARY_INPUT		'{'
#define API_PRIMARY_OUTPUT		'}'
#define API_SECONDARY_INPUT		'('
#define API_SECONDARY_OUTPUT	')'

GMT_LOCAL int api_key_to_family (void *API, char *key, int *family, int *geometry) {
	/* Assign direction, family, and geometry based on the key.
	   Note: No Vector or Matrix here since those always masquarade as DATASET in modules. */

	switch (key[K_FAMILY]) {	/* 2nd char contains the data type code */
		case 'G':
			*family = GMT_IS_GRID;
			*geometry = GMT_IS_SURFACE;
			break;
		case 'P':
			*family = GMT_IS_DATASET;
			*geometry = GMT_IS_POLY;
			break;
		case 'L':
			*family = GMT_IS_DATASET;
			*geometry = GMT_IS_LINE;
			break;
		case 'D':
			*family = GMT_IS_DATASET;
			*geometry = GMT_IS_POINT;
			break;
		case 'C':
			*family = GMT_IS_PALETTE;
			*geometry = GMT_IS_NONE;
			break;
		case 'T':
			*family = GMT_IS_TEXTSET;
			*geometry = GMT_IS_NONE;
			break;
		case 'I':
			*family = GMT_IS_IMAGE;
			*geometry = GMT_IS_SURFACE;
			break;
		case 'X':
			*family = GMT_IS_POSTSCRIPT;
			*geometry = GMT_IS_NONE;
			break;
		case '-':
			*family = GMT_IS_NONE;
			*geometry = GMT_IS_NONE;
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "api_key_to_family: INTERNAL ERROR: key family (%c) not recognized\n", key[K_FAMILY]);
			return GMT_NOTSET;
			break;
	}

	/* Third key character contains the in/out code */
	return ((key[K_DIR] == API_SECONDARY_OUTPUT || key[K_DIR] == API_PRIMARY_OUTPUT) ? GMT_OUT : GMT_IN);	/* Return the direction of the i/o */
}

GMT_LOCAL char **api_process_keys (void *API, const char *string, char type, struct GMT_OPTION *head, int *n_to_add, unsigned int *n_items) {
	/* Turn the comma-separated list of 3-char codes in string into an array of such codes.
 	 * In the process, replace any ?-types with the selected type if type is not 0.
	 * We return the array of strings and its number (n_items). */
	size_t len, k, kk, n;
	int o_id = GMT_NOTSET, family = GMT_NOTSET, geometry = GMT_NOTSET;
	bool change_type = false;
	char **s = NULL, *next = NULL, *tmp = NULL, magic = 0, revised[GMT_LEN64] = {""};
	struct GMT_OPTION *opt = NULL;
	*n_items = 0;	/* No keys yet */

	for (k = 0; k < GMT_N_FAMILIES; k++) n_to_add[k] = GMT_NOTSET;	/* Initially no input counts */
	if (!string) return NULL;	/* Got NULL, so just give up */
	len = strlen (string);		/* Get the length of this item */
	if (len == 0) return NULL;	/* Got no characters, so give up */
	tmp = strdup (string);		/* Get a working copy of string */
	/* Replace unknown types (marked as ?) in tmp with selected type give by input variable "type" */
	if (type) {	/* Got a nonzero type */
		for (k = 0; k < strlen (tmp); k++)
			if (tmp[k] == '?') tmp[k] = type;
	}
	/* Count the number of items (start n at 1 since there are one less comma than items) */
	for (k = 0, n = 1; k < len; k++)
		if (tmp[k] == ',') n++;
	/* Allocate and populate the character array, then return it and n_items */
	s = (char **) calloc (n, sizeof (char *));
	k = 0;
	while ((next = strsep (&tmp, ",")) != NULL) {	/* Get each comma-separated key */
		if (strlen (next) < 3) {
			GMT_Report (API, GMT_MSG_NORMAL,
			            "api_process_keys: INTERNAL ERROR: key %s contains less than 3 characters\n", next);
			continue;
		}
		if (strchr (next, '!')) {	/* Type did not get determined in GMT_Encode_Options so key is skipped */
			GMT_Report (API, GMT_MSG_DEBUG,
			            "api_process_keys: key %s contains type = ! so we skip it\n", next);
			n--;
			continue;
		}
		s[k] = strdup (next);
		if (next[K_DIR] == API_PRIMARY_OUTPUT) {	/* Identified primary output key */
			if (o_id >= 0)	/* Already had a primary output key */
				GMT_Report (API, GMT_MSG_NORMAL,
				            "api_process_keys: INTERNAL ERROR: keys %s contain more than one primary output key\n", string);
			else
				o_id = (int)k;
		}
		k++;
	}

	/* While processing the array we also determine the key # for the primary output (if there is one) */
	for (k = 0; k < n; k++) {	/* Check for presence of any of the magic X,Y,Z keys */
		if (s[k][K_OPT] == '-') {	/* Key letter X missing: Means that option -Y, if given, changes the type of input|output */
			/* Must first determine which data type we are dealing with via -Y<type> */
			if ((opt = GMT_Find_Option (API, s[k][K_FAMILY], head))) {	/* A -Y<type> option was passed to the module */
				type = (char)toupper (opt->arg[0]);	/* Find type and replace any ? in keys with this type in uppercase (DGCITP) in api_process_keys below */
				if (!strchr ("DGCITP", type)) {
					GMT_Report (API, GMT_MSG_NORMAL, "api_process_keys: INTERNAL ERROR: No or bad data type given to read|write (%c)\n", type);
					return_null (NULL, GMT_NOT_A_VALID_TYPE);	/* Unknown type */
				}
				if (type == 'P') type = 'X';	/* We use X for PostScript internally since P may stand for polygon... */
				for (kk = 0; kk < n; kk++) {	/* Do the substitution for all keys that matches ? */
					if (s[kk][K_FAMILY] == '?' && strchr ("-({", s[kk][K_DIR])) s[kk][K_FAMILY] = type;	/* Want input to handle this type of data */
					if (s[kk][K_FAMILY] == '?' && strchr ("-)}", s[kk][K_DIR])) s[kk][K_FAMILY] = type;	/* Want output to handle this type of data */
				}
			}
			else
				GMT_Report (API, GMT_MSG_NORMAL,
				            "api_process_keys: INTERNAL ERROR: Required runtime type-getting option (-%c) was not given\n", s[k][K_FAMILY]);
			gmt_M_str_free (s[k]);		/* Free the inactive key that has now served its purpose */
		}
		else if (s[k][K_FAMILY] == '-') {	/* Key letter Y missing: Means that -X, if given, changes primary input|output set by -Z to secondary (i.e., not required) */
			/* However, if +<mod> is appended then the primary input setting is left as is */
			if ((opt = GMT_Find_Option (API, s[k][K_OPT], head))) {	/* Got the option that removes the requirement of an input or output dataset */
				if (!(s[k][3] == '+' && strstr (opt->arg, &s[k][3]))) {	/* Got the option and no modifier to turn it off */
					for (kk = 0; kk < n; kk++) {	/* Change all primary input|output flags to secondary, depending on Z */
						if (!s[kk]) continue;		/* A previously processed/freed key */
						if (s[kk][K_OPT] != s[k][K_DIR]) continue;		/* Not the "-Z "option */
						if (s[kk][K_DIR] == API_PRIMARY_INPUT) s[kk][K_DIR] = API_SECONDARY_INPUT;		/* No longer an implicit input */
						else if (s[kk][K_DIR] == API_PRIMARY_OUTPUT) s[kk][K_DIR] = API_SECONDARY_OUTPUT;	/* No longer an implicit output */
					}
				}
			}
			gmt_M_str_free (s[k]);		/* Free the inactive key that has served its purpose */
		}
		else if (!strchr ("{}()-", s[k][K_DIR])) {	/* Key letter Z not in {|(|}|)|-: which means that option -Z, if given, changes the type of primary output to Y */
			/* E.g, pscoast has >DM and this turns >X} to >D} only when -M is used.  Also, modifiers may be involved.
			   e.g, gmtspatial : New key ">TN+r" means if -N+r is given then set >T}.  Just giving -N will not trigger the change.
			   e.g., pscoast ">TE+w-rR" means if -E is given with modifier +w _and_ one of +r or +R is then set to >T}.
			   If X is not - then we will find the other KEY with X and select that as the one to change; this could
			   be used to change the primary INPUT type.  For instance, grdimage expects grid input (<G{+) but with
			   magic sequence <ID we change <G{+ to <I{+.  */
			magic = s[k][K_DIR];
			if ((opt = GMT_Find_Option (API, magic, head))) {	/* Got the magic option that changes output type */
				char modifier[3] = {'+', '?', 0};	/* We will replace ? with an actual modifier */
				size_t this_k;
				if (o_id == GMT_NOTSET)
					GMT_Report (API, GMT_MSG_NORMAL, "api_process_keys: INTERNAL ERROR: No primary output identified but magic Z key present\n");
				/* Check if modifier(s) were given also and that one of them were selected */
				if (strlen (s[k]) > 3) {	/* Not enough to just find option, must examine the modifiers */
					/* Full syntax: XYZ+abc...-def...: We do the substitution of output type to Y only if
					 * 1. -Z is given
					 * 2. -Z contains ALL the modifiers +a, +b, +c, ...
					 * 3. -Z contains AT LEAST ONE of the modifers +d, +e, +f.
					 */
					unsigned int kase = 0, count[2] = {0, 0}, given[2] = {0, 0};
					change_type = false;
					for (kk = 3; s[k][kk]; kk++) {	/* Examine characters in the modifier string */
						if (strchr ("-+", s[k][kk])) {	/* Start of all (+) versus at least one (-) */
							kase = (s[k][kk] == '-') ? 0 : 1;	/* Set kase and go to next letter */
							continue;
						}
						count[kase]++;	/* How many AND and how many OR modifiers (depending on kase) */
						modifier[1] = s[k][kk];	/* Set current modifier */
						if (strstr (opt->arg, modifier)) given[kase]++;	/* Match found with given option */
					}
					/* Only change the key if we found all the AND modifiers and at least one of the OR modifiers (if any were given) */
					if ((count[0] == 0 || (count[0] && given[0])) && count[1] == given[1]) change_type = true;
				}
				else	/* true since we found the option and no modifiers were given */
					change_type = true;
				if (s[k][K_OPT] != '-') {	/* Find the relevant option to change [primary output key] */
					char match = (s[k][K_OPT] == '<') ? API_PRIMARY_INPUT : API_PRIMARY_OUTPUT;
					for (kk = 0, this_k = n; kk < n; kk++) {
						if (kk == k || s[kk] == NULL) continue;
						if (s[kk][K_OPT] == s[k][K_OPT] && s[kk][K_DIR] == match)
							this_k = kk;
					}
					if (this_k == n) this_k = o_id;
				}
				else	/* Select the primary output key */
					this_k = o_id;
				if (change_type) {
					if (strchr ("{<", s[this_k][K_DIR])) {
						int new_family = 0, old_family = 0;
						(void)api_key_to_family (API, s[k], &new_family, &geometry);
						(void)api_key_to_family (API, s[this_k], &old_family, &geometry);
						if (new_family != old_family) gmt_M_int_swap (n_to_add[new_family], n_to_add[old_family]);	/* Must swap our counts */
					}
					s[this_k][K_FAMILY] = s[k][K_FAMILY];	/* Required input/output now implies this data type */
					s[this_k][K_OPT]    = s[k][K_OPT];	/* Required input/output now implies this option */
				}
			}
			gmt_M_str_free (s[k]);		/* Free the inactive key that has served its purpose */
		}
		else if (s[k][K_DIR] == API_PRIMARY_INPUT) {	/* Non-magic key: This one identified a primary input key */
			(void)api_key_to_family (API, s[k], &family, &geometry);	/* Get datatype, and geometry, then set how many are requested */
			if (family != GMT_NOTSET) {	/* Safeguard: If family not found then we don't want to crash below... */
				if (s[k][K_DIR+1])	/* Gave an argument: This is either a number (a specific count) or + (1 or more) */
					n_to_add[family] = (s[k][K_DIR+1] == '+') ? GMTAPI_UNLIMITED : atoi (&s[k][K_DIR+1]);
				else
					n_to_add[family] = (family == GMT_IS_DATASET || family == GMT_IS_TEXTSET) ? GMTAPI_UNLIMITED : 1;
			}
		}
	}
	/* Shuffle away any NULL entries as a result of magic key processing */
	for (k = kk = 0; k < n; k++) {
		if (s[k]) {	/* Must keep this guy */
			if (k > kk) s[kk] = s[k];
			kk++;
		}
	}
	n = kk;	/* May have lost some NULLs.  Make a revised string for debug output */
	for (k = 0; k < n; k++) {
		strcat (revised, ",");
		strcat (revised, s[k]);
	}
	if (revised[0]) GMT_Report (API, GMT_MSG_DEBUG, "api_process_keys: Revised keys string is %s\n", &revised[1]);
	*n_items = (unsigned int)n;	/* Total number of remaining keys for this module */
	gmt_M_str_free (tmp);
	return s;	/* The array of remaining keys */
}

GMT_LOCAL int api_get_key (void *API, char option, char *keys[], int n_keys) {
	/* Returns the position in the keys array that matches this option, or GMT_NOTSET if not found */
	int k;
	if (n_keys && keys == NULL)
		GMT_Report (API, GMT_MSG_NORMAL, "api_get_key: INTERNAL ERROR: keys array is NULL but n_keys = %d\n", n_keys);
	for (k = 0; keys && k < n_keys; k++) if (keys[k][K_OPT] == option) return (k);
	return (GMT_NOTSET);
}

GMT_LOCAL bool api_found_marker (char *text) {
	/* A single questionmark and nothing else indicates a file marker */
	if (text[0] == '?' && text[1] == '\0') return true;
	return false;	/* Not found */
}

GMT_LOCAL unsigned int api_determine_dimension (struct GMTAPI_CTRL *API, char *text) {
	/* Examine greenspline's -R option to learn the dimensionality of the domain (1, 2, or 3) */
	unsigned int n_slashes = 0;
	size_t k;
	const size_t s_length = strlen(text);

	/* First catch the simple -R? which means a grid is passed by the API, hence dimension is 2 */
	if (text[0] == '?' && text[1] == '\0') return 2;	/* A marker means a grid only, so done */
	for (k = 0; k < s_length; k++)
		if (text[k] == '/') n_slashes++;			/* Count slashes just in case */
	if ((text[0] == 'g' || text[0] == 'd') && (text[1] == '\0' || text[1] == '/')) {	/* Got -Rg or -Rd, possibly with trailing /zmin/zmax */
		if (text[1] == '\0') return 2;	/* Got -Rg or -Rd and no more */
		if (n_slashes == 2) return 3;	/* Got -Rg/zmin/zmax or -Rd/zmin/zmax */
		GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -R option: Give 2, 4, or 6 coordinates, a gridfile, or use -Rd|g[/zmin/zmax]\n");
		return 0;
	}
	if (!gmt_access (API->GMT, text, R_OK))	/* Gave a readable file, we assume it is a grid since that is all that is allowed */
		return 2;
	/* Only get here if the above cases did not trip */
	if (!(n_slashes == 1 || n_slashes == 3 || n_slashes == 5)) {
		GMT_Report (API, GMT_MSG_NORMAL, "Syntax error -R option: Give 2, 4, or 6 coordinates\n");
		return 0;
	}
	return ((n_slashes + 1) / 2);	/* Turns 1,3,5 into 1,2,3 */
}

/*! . */
GMT_LOCAL void *api_retrieve_data (void *V_API, int object_ID) {
	/* Function to return pointer to the container for a registered data set.
	 * Typically used when we wish a module to "write" its results to a memory
	 * location that we wish to access from the calling program.  The procedure
	 * is to use GMT_Register_IO with GMT_REF|COPY|READONLY and GMT_OUT but use
	 * NULL as the source/destination.  Data are "written" by GMT allocating a
	 * output container and updating the objects->resource pointer to this container.
	 * api_retrieve_data simply returns that pointer given the registered ID.
	 */

	int item;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);

	/* Determine the item in the object list that matches this object_ID */
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET) {
		return_null (API, API->error);
	}
	/* Make sure the resource is present */
	if (API->object[item]->resource == NULL) {
		return_null (API, GMT_PTR_IS_NULL);
	}
	/* Make sure the data pointer has not been set */
	if (API->object[item]->data) {
		return_null (API, GMT_PTR_NOT_NULL);
	}
	/* Assign data from resource and wipe resource pointer */
	API->object[item]->data = API->object[item]->resource;
	API->object[item]->resource = NULL;

#ifdef DEBUG
	api_list_objects (API, "api_retrieve_data");
#endif
	return (API->object[item]->data);	/* Return pointer to the data container */
}

/*! . */
GMT_LOCAL int api_begin_io (struct GMTAPI_CTRL *API, unsigned int direction) {
	/* Initializes the i/o mechanism for either input or output (depends on direction).
	 * api_begin_io must be called before any bulk data i/o is allowed.
	 * direction:	Either GMT_IN or GMT_OUT.
	 * Returns:	false if successful, true if error.
	 */

	struct GMT_CTRL *GMT = NULL;
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (!API->registered[direction])
		GMT_Report (API, GMT_MSG_DEBUG, "api_begin_io: Warning: No %s resources registered\n", GMT_direction[direction]);
	/* Passed basic sanity checks */
	GMT = API->GMT;
	API->io_mode[direction] = GMTAPI_BY_SET;
	API->io_enabled[direction] = true;	/* OK to access resources */
	GMT->current.io.ogr = GMT_OGR_UNKNOWN;
	GMT->current.io.read_mixed = false;
	GMT->current.io.need_previous = (GMT->common.g.active || GMT->current.io.skip_duplicates);
	GMT->current.io.segment_header[0] = GMT->current.io.record[0] = 0;
	GMT_Report (API, GMT_MSG_DEBUG, "api_begin_io: %s resource access is now enabled [container]\n", GMT_direction[direction]);

	return (GMT_OK);	/* No error encountered */
}

/* Mapping of internal [row][col] indices to a single 1-D index.
 * Internally, row and col both starts at 0.  These will be accessed
 * via pointers to these functions, hence they are not macros.
 * They apply to GMT_MATRIX items, NOT grids/images with pads.
 */

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_c_normal (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for C normal row-major grid */
	return ((row * dim) + col);	/* Normal scanline grid */
}

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_c_cplx_real (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for C complex row-major grid, real component */
	return (2ULL*(row * dim) + col);	/* Complex scanline grid, real(1) component */
}

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_c_cplx_imag (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for C complex row-major grid, imaginary component */
	return (2ULL*(row * dim) + col + 1ULL);	/* Complex grid, imag(2) component */
}

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_f_normal (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for Fortran column-major grid */
	return ((col * dim) + row);
}

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_f_cplx_real (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for Fortran complex column-major grid, real component */
	return (2ULL*(col * dim) + row);	/* Complex grid, real(1) */
}

/*! . */
GMT_LOCAL uint64_t api_2d_to_index_f_cplx_imag (uint64_t row, uint64_t col, uint64_t dim) {
	/* Maps (row,col) to 1-D index for Fortran complex column-major grid, imaginary component  */
	return (2ULL*(col * dim) + row + 1ULL);	/* Complex grid, imag(2) component */
}

/*! . */
GMT_LOCAL p_func_uint64_t api_get_2d_to_index (struct GMTAPI_CTRL *API, enum GMT_enum_fmt shape, unsigned int mode) {
	/* Return pointer to the required 2D-index function above for MATRIX.  Here
	 * shape is either GMT_IS_ROW_FORMAT (C) or GMT_IS_COL_FORMAT (Fortran);
	 * mode is either 0 (regular grid), GMT_GRID_IS_COMPLEX_REAL (complex real) or GMT_GRID_IS_COMPLEX_IMAG (complex imag)
	 */
	p_func_uint64_t p = NULL;

	switch (mode & GMT_GRID_IS_COMPLEX_MASK) {
		case GMT_GRID_IS_REAL:
			p = (shape == GMT_IS_ROW_FORMAT) ? api_2d_to_index_c_normal : api_2d_to_index_f_normal;
			break;
		case GMT_GRID_IS_COMPLEX_REAL:
			p = (shape == GMT_IS_ROW_FORMAT) ? api_2d_to_index_c_cplx_real : api_2d_to_index_f_cplx_real;
			break;
		case GMT_GRID_IS_COMPLEX_IMAG:
			p = (shape == GMT_IS_ROW_FORMAT) ? api_2d_to_index_c_cplx_imag : api_2d_to_index_f_cplx_imag;
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "api_get_2d_to_index: Illegal mode passed - aborting\n");
			api_exit (API, GMT_RUNTIME_ERROR); return (NULL);
	}
	return (p);
}

#if 0	/* Unused at the present time */
GMT_LOCAL void api_index_to_2d_c (int *row, int *col, size_t index, int dim, int mode) {
	/* Maps 1-D index to (row,col) for C */
	if (mode) index /= 2;
	*col = (index % dim);
	*row = (index / dim);
}

GMT_LOCAL void api_index_to_2d_f (int *row, int *col, size_t index, int dim, int mode) {
	/* Maps 1-D index to (row,col) for Fortran */
	if (mode) index /= 2;
	*col = (index / dim);
	*row = (index % dim);
}
#endif

/* Mapping of internal [row][col][layer] indices to a single 1-D index for images.
 * Internally, row and col starts at 0.  These will be accessed
 * via pointers to these functions, hence they are not macros.
 */

GMT_LOCAL uint64_t api_get_index_from_TRB (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index of an array with a band-interleaved layout RRR...RGGG...GBBB...B */
	return (h->pad[XLO] + col) + ((row + h->pad[YHI]) * h->mx) + (layer * h->size);
}

GMT_LOCAL uint64_t api_get_index_from_TRP (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index of an array with a pixel-interleaved layout RGBRGBRGB...*/
	return ((h->pad[XLO] + col) * h->n_bands) + layer + ((row + h->pad[YHI]) * h->mx * h->n_bands);
}

GMT_LOCAL uint64_t api_get_index_from_TRL (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index of an array with a line-interleaved layout R...RG..GB...BR...RG...GB...B...*/
	return (h->pad[XLO] + col) + (layer * h->mx) + ((row + h->pad[YHI]) * h->mx * h->n_bands);
}

GMT_LOCAL uint64_t api_get_index_from_TRS (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index of an default GMT grid */
	gmt_M_unused(layer);
	return (gmt_M_ijp (h, row, col));
}

GMT_LOCAL uint64_t api_get_index_from_TRR (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index to the real component of an default complex GMT grid */
	gmt_M_unused(layer);
	return (2ULL*gmt_M_ijp (h, row, col));
}

GMT_LOCAL uint64_t api_get_index_from_TRI (struct GMT_GRID_HEADER *h, uint64_t row, uint64_t col, uint64_t layer) {
	/* Get linear index to the imag component of an default complex GMT grid */
	gmt_M_unused(layer);
	return (2ULL*gmt_M_ijp (h, row, col)+1ULL);
}

/*! . */
GMT_LOCAL unsigned int api_decode_layout (struct GMTAPI_CTRL *API, const char *code, enum GMT_enum_family *family) {
	/* Convert the 3-letter grid/image layout code to a single integer mode.
	 * Defaults are TRS for grids and TRB for images. */
	unsigned int bits = 0;	/* Default value */
	*family = GMT_IS_IMAGE;	/* Default value, may be changed later */
	switch (code[0]) {	/* Char 1: The Y direction */
		case 'T':	break;			 /* Top-to-bottom [Default] */
		case 'B':	bits = 1; break; /* Bottom-to-top */
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Illegal code [%c] for y-direction grid/image layout.  Must be T or B\n", code[0]);
			break;
	}
	switch (code[1]) {	/* Char 2: The storage mode (rows vs columns) */
		case 'R':	break;		 	  /* rows, i.e., scanlines [Default] */
		case 'C':	bits |= 2; break; /* columns (e.g., Fortran) */
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Illegal code [%c] for grid/image storage mode.  Must be R or C\n", code[1]);
			break;
	}
	switch (code[2]) {	/* Char 3: Grids: Single, complex-Real, complex-Imag.  Images: band interleaving mode B|L|P */
		case 'S':	*family = GMT_IS_GRID; break;	/* Single-valued grid [Default] */
		case 'R':	bits |= 4; *family = GMT_IS_GRID; break;	/* Real component of complex grid */
		case 'I':	bits |= 8; *family = GMT_IS_GRID; break;	/* Imaginary component of complex grid */
		case 'B':	break;			/* r/g/b separated into three bands (layers) */
		case 'L':	bits |= 4; break;	/* r/g/b separated into three lines */
		case 'P':	bits |= 8; break;	/* r/g/b separated into three values per pixel */
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Illegal code [%c] for type of grid or image layout.  Must be SRI (grids) or BLP (images)\n", code[1]);
			break;
	}
	return (bits);
}

/*! . */
GMT_LOCAL int api_init_grid (struct GMTAPI_CTRL *API, struct GMT_OPTION *opt, uint64_t dim[], double *range, double *inc, int registration, unsigned int mode, unsigned int direction, struct GMT_GRID *G) {
	if (direction == GMT_OUT) return (GMT_OK);	/* OK for creating a blank container for output */
	gmtgrdio_init_grdheader (API->GMT, G->header, opt, dim, range, inc, registration, mode);
	return (GMT_OK);
}

/*! . */
GMT_LOCAL int api_init_image (struct GMTAPI_CTRL *API, struct GMT_OPTION *opt, uint64_t dim[], double *range, double *inc, int registration, unsigned int mode, unsigned int direction, struct GMT_IMAGE *I) {
	int alpha;
	if (direction == GMT_OUT) return (GMT_OK);	/* OK for creating blank container for output */
	alpha = (dim && (dim[GMT_Z] == 2 || dim[GMT_Z] == 4));	/* Must allocate alpha array later */
	if (alpha) dim[GMT_Z]--;	/* Remove this flag before grdheader is set */
	gmtgrdio_init_grdheader (API->GMT, I->header, opt, dim, range, inc, registration, mode);
	if (alpha) dim[GMT_Z]++;	/* Restore */
	return (GMT_OK);
}

/*! . */
GMT_LOCAL int api_init_matrix (struct GMTAPI_CTRL *API, uint64_t dim[], double *range, double *inc, int registration, unsigned int mode, unsigned int direction, struct GMT_MATRIX *M) {
	double off = 0.5 * registration;
	int error;
	unsigned int dims = (M->n_layers > 1) ? 3 : 2;
	size_t size = 0;
	GMT_Report (API, GMT_MSG_DEBUG, "Initializing a matrix for handing external %s [mode = %u]\n", GMT_direction[direction], mode);
	if (direction == GMT_OUT) return (GMT_OK);	/* OK for creating blank container for output */
	if (full_region (range) && (dims == 2 || (!range || range[ZLO] == range[ZHI]))) {	/* Not an equidistant vector arrangement, use dim */
		double dummy_range[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};	/* Flag vector as such */
		gmt_M_memcpy (M->range, dummy_range, 2 * dims, double);
		M->n_rows = dim[GMTAPI_DIM_ROW];
		M->n_columns = dim[GMTAPI_DIM_COL];
	}
	else {	/* Was apparently given valid range and inc */
		if (!inc || (inc[GMT_X] == 0.0 && inc[GMT_Y] == 0.0)) return (GMT_VALUE_NOT_SET);
		gmt_M_memcpy (M->range, range, 2 * dims, double);
		M->n_rows    = gmt_M_get_n (API->GMT, range[YLO], range[YHI], inc[GMT_Y], off);
		M->n_columns = gmt_M_get_n (API->GMT, range[XLO], range[XHI], inc[GMT_X], off);
	}
	M->type = dim[3];	/* Use selected data type for export */
	M->dim = (M->shape == GMT_IS_ROW_FORMAT) ? M->n_columns : M->n_rows;
	size = M->n_rows * M->n_columns * M->n_layers;	/* Size in bytes of the initial matrix allocation */
	if (size && (mode & GMT_CONTAINER_ONLY) == 0) {	/* Must allocate memory */
		if ((error = gmtlib_alloc_univector (API->GMT, &(M->data), M->type, size)) != GMT_OK)
			return (error);
		M->alloc_mode = GMT_ALLOC_INTERNALLY;
	}
	return (GMT_OK);
}

/*! . */
GMT_LOCAL int api_init_vector (struct GMTAPI_CTRL *API, uint64_t dim[], double *range, double *inc, int registration, unsigned int mode, unsigned int direction, struct GMT_VECTOR *V) {
	int error;
	uint64_t col;
	GMT_Report (API, GMT_MSG_DEBUG, "Initializing a vector for handing external %s\n", GMT_direction[direction]);
	if (direction == GMT_OUT) return (GMT_OK);	/* OK for creating blank container for output */
	if (dim[GMTAPI_DIM_COL] == 0 && direction == GMT_IN) return (GMT_VALUE_NOT_SET);	/* Must know the number of columns to do this */
	if (range == NULL || (range[XLO] == range[XHI])) {	/* Not an equidistant vector arrangement, use dim */
		double dummy_range[2] = {0.0, 0.0};	/* Flag vector as such */
		V->n_rows = dim[GMTAPI_DIM_ROW];		/* If so, n_rows is passed via dim[GMTAPI_DIM_ROW], unless it is GMT_OUT when it is zero */
		gmt_M_memcpy (V->range, dummy_range, 2, double);
	}
	else {	/* Equidistant vector */
		double off = 0.5 * registration;
		if (!inc || inc[GMT_X] == 0.0) return (GMT_VALUE_NOT_SET);
		V->n_rows = gmt_M_get_n (API->GMT, range[XLO], range[XHI], inc[GMT_X], off);
		gmt_M_memcpy (V->range, range, 2, double);
	}
	for (col = 0; col < V->n_columns; col++)	/* Set the same export data type for all vectors */
		V->type[col] = dim[2];
	if (V->n_rows && (mode & GMT_CONTAINER_ONLY) == 0) {	/* Must allocate space */
		if ((error = api_alloc_vectors (API->GMT, V, V->n_rows)) != GMT_OK)
			return (error);
		V->alloc_mode = GMT_ALLOC_INTERNALLY;
	}

	return (GMT_OK);
}

/*! . */
GMT_LOCAL double * api_grid_coord (struct GMTAPI_CTRL *API, int dim, struct GMT_GRID *G) {
	return (gmt_grd_coord (API->GMT, G->header, dim));
}

/*! . */
GMT_LOCAL double * api_image_coord (struct GMTAPI_CTRL *API, int dim, struct GMT_IMAGE *I) {
	return (gmt_grd_coord (API->GMT, I->header, dim));
}

/*! . */
GMT_LOCAL double * api_matrix_coord (struct GMTAPI_CTRL *API, int dim, struct GMT_MATRIX *M) {
	/* Allocate and compute coordinates along one dimension of a matrix */
	double *coord = NULL, off, inc;
	unsigned int min, max;
	uint64_t k, n;

	if (M->n_layers <= 1 && dim == GMT_Z) return (NULL);	/* No z-dimension */
	n = (dim == GMT_X) ? M->n_columns : ((dim == GMT_Y) ? M->n_rows : M->n_layers);
	min = 2*dim, max = 2*dim + 1;	/* Indices into the min/max triplets in range */
	coord = gmt_M_memory (API->GMT, NULL, n, double);
	off = 0.5 * M->registration;
	inc = gmt_M_get_inc (API->GMT, M->range[min], M->range[max], n, M->registration);
	for (k = 0; k < n; k++) coord[k] = gmt_M_col_to_x (API->GMT, k, M->range[min], M->range[max], inc, off, n);
	return (coord);
}

/*! . */
GMT_LOCAL double * api_vector_coord (struct GMTAPI_CTRL *API, int dim, struct GMT_VECTOR *V) {
	/* Allocate and compute coordinates for a vector, if equidistantly defined */
	unsigned int k;
	double *coord = NULL, off, inc;
	GMT_Report (API, GMT_MSG_DEBUG, "api_vector_coord called: dim = %d\n", dim);
	if (V->range[0] == 0.0 && V->range[1] == 0.0) return (NULL);	/* Not an equidistant vector */
	coord = gmt_M_memory (API->GMT, NULL, V->n_rows, double);
	off = 0.5 * V->registration;
	inc = gmt_M_get_inc (API->GMT, V->range[0], V->range[1], V->n_rows, V->registration);
	for (k = 0; k < V->n_rows; k++) coord[k] = gmt_M_col_to_x (API->GMT, k, V->range[0], V->range[1], inc, off, V->n_rows);
	return (coord);
}

/*! . */
GMT_LOCAL void api_grdheader_to_matrixinfo (struct GMT_GRID_HEADER *h, struct GMT_MATRIX *M_obj) {
	/* Packs the necessary items of the grid header into the matrix parameters */
	M_obj->n_columns = h->n_columns;
	M_obj->n_rows = h->n_rows;
	M_obj->registration = h->registration;
	gmt_M_memcpy (M_obj->range, h->wesn, 4, double);
}

/*! . */
GMT_LOCAL void api_matrixinfo_to_grdheader (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *h, struct GMT_MATRIX *M_obj) {
	/* Unpacks the necessary items into the grid header from the matrix parameters */
	gmt_M_unused(GMT);
	h->n_columns = (unsigned int)M_obj->n_columns;
	h->n_rows = (unsigned int)M_obj->n_rows;
	h->registration = M_obj->registration;
	gmt_M_memcpy (h->wesn, M_obj->range, 4, double);
	/* Compute xy_off and increments */
	h->xy_off = (h->registration == GMT_GRID_NODE_REG) ? 0.0 : 0.5;
	h->inc[GMT_X] = gmt_M_get_inc (GMT, h->wesn[XLO], h->wesn[XHI], h->n_columns, h->registration);
	h->inc[GMT_Y] = gmt_M_get_inc (GMT, h->wesn[YLO], h->wesn[YHI], h->n_rows, h->registration);
}

/*! . */
GMT_LOCAL bool api_adjust_grdpadding (struct GMT_GRID_HEADER *h, unsigned int *pad) {
	/* Compares current grid pad status to output pad requested.  If we need
	 * to adjust a pad we return true here, otherwise false. */
	unsigned int side;

	for (side = 0; side < 4; side++) if (h->pad[side] != pad[side]) return (true);
	return (false);
}

/*! . */
GMT_LOCAL size_t api_set_grdarray_size (struct GMT_CTRL *GMT, struct GMT_GRID_HEADER *h, unsigned int mode, double *wesn) {
	/* Determines size of grid given grid spacing and grid domain in h.
 	 * However, if wesn is given and not empty we compute size using the sub-region instead.
 	 * Finally, the current pad is used when calculating the grid size.
	 * NOTE: This function leaves h unchanged by testing on a temporary header. */
	struct GMT_GRID_HEADER *h_tmp = NULL;
	size_t size;

	/* Must duplicate header and possibly reset wesn, then set pad and recalculate all dims */
	h_tmp = gmt_M_memory (GMT, NULL, 1, struct GMT_GRID_HEADER);
	gmt_M_memcpy (h_tmp, h, 1, struct GMT_GRID_HEADER);
	h_tmp->complex_mode = (mode & GMT_GRID_IS_COMPLEX_MASK);	/* Set the mode-to-be so that if complex the size is doubled */

	if (!full_region (wesn)) {
		gmt_M_memcpy (h_tmp->wesn, wesn, 4, double);    /* Use wesn instead of header info */
		gmt_adjust_loose_wesn (GMT, wesn, h);           /* Subset requested; make sure wesn matches header spacing */
		gmt_M_memcpy(h_tmp->wesn, wesn, 4, double);	    /* And update the eventually adjusted wesn */
	}
	gmt_M_grd_setpad (GMT, h_tmp, GMT->current.io.pad);	/* Use the system pad setting by default */
	gmt_set_grddim (GMT, h_tmp);				/* Computes all integer parameters */
	size = h_tmp->size;					/* This is the size needed to hold grid + padding */
	gmt_M_free (GMT, h_tmp);
	return (size);
}

/*! . */
GMT_LOCAL int api_open_grd (struct GMT_CTRL *GMT, char *file, struct GMT_GRID *G, char mode, unsigned int access_mode) {
	/* Read or write the header structure and initialize row-by-row machinery for grids.
	 * We fill the GMT_GRID_ROWBYROW structure with all the required information.
	 * mode can be w or r.  Upper case W or R refers to headerless
	 * grdraster-type files.  The access_mode dictates if we automatically advance
	 * row counter to next row after read/write or if we use the rec_no to seek
	 * first.
	 */

	int r_w, err;
	bool header = true, magic = true, alloc = false;
	int cdf_mode[3] = { NC_NOWRITE, NC_WRITE, NC_WRITE};	/* These MUST be ints */
	char *bin_mode[3] = { "rb", "rb+", "wb"};
	char *fmt = NULL;
	struct GMT_GRID_ROWBYROW *R = api_get_rbr_ptr (G->extra);	/* Shorthand to row-by-row book-keeping structure */

	if (mode == 'r' || mode == 'R') {	/* Open file for reading */
		if (mode == 'R') {	/* File has no header; can only work if G->header has been set already, somehow */
			header = false;
			if (G->header->n_columns == 0 || G->header->n_rows == 0) {
				GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unable to read header-less grid file %s without a preset header structure\n", file);
				return (GMT_GRDIO_OPEN_FAILED);
			}
		}
		r_w = 0;	mode = 'r';
	}
	else if (mode == 'W') {	/* Write headerless grid */
		r_w = 2;	mode = 'w';
		header = magic = false;
	}
	else {	/* Regular writing of grid with header */
		r_w = 1;
		magic = false;
	}
	if (header) {
		if (mode == 'r' && !R->open)	/* First time reading the info */
			gmtlib_read_grd_info (GMT, file, G->header);
		else if (R->open)		/* Coming back to update the header */
			gmt_update_grd_info (GMT, file, G->header);
		else				/* First time writing the header */
			gmtlib_write_grd_info (GMT, file, G->header);
	}
	else /* Fallback to existing header */
		gmt_M_err_trap (gmt_grd_get_format (GMT, file, G->header, magic));
	if (R->open) return (GMT_NOERROR);	/* Already set the first time */
	fmt = GMT->session.grdformat[G->header->type];
	if (fmt[0] == 'c') {		/* Open netCDF file, old format */
		gmt_M_err_trap (nc_open (G->header->name, cdf_mode[r_w], &R->fid));
		R->edge[0] = G->header->n_columns;
		R->start[0] = 0;
		R->start[1] = 0;
	}
	else if (fmt[0] == 'n') {	/* Open netCDF file, COARDS-compliant format */
		gmt_M_err_trap (nc_open (G->header->name, cdf_mode[r_w], &R->fid));
		R->edge[0] = 1;
		R->edge[1] = G->header->n_columns;
		R->start[0] = G->header->row_order == k_nc_start_north ? 0 : G->header->n_rows-1;
		R->start[1] = 0;
	}
	else {		/* Regular binary file with/w.o standard GMT header, or Sun rasterfile */
		if (r_w == 0) {	/* Open for plain reading */
			if ((R->fp = gmt_fopen (GMT, G->header->name, bin_mode[0])) == NULL)
				return (GMT_GRDIO_OPEN_FAILED);
		}
		else if ((R->fp = gmt_fopen (GMT, G->header->name, bin_mode[r_w])) == NULL)
			return (GMT_GRDIO_CREATE_FAILED);
		/* Seek past the grid header, unless there is none */
		if (header && fseek (R->fp, (off_t)GMT_GRID_HEADER_SIZE, SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
		alloc = (fmt[1] != 'f');	/* Only need to allocate the v_row array if grid is not float */
	}

	R->size = gmt_grd_data_size (GMT, G->header->type, &G->header->nan_value);
	R->check = !isnan (G->header->nan_value);
	R->open = true;

	if (fmt[1] == 'm')	/* Bit mask */
		R->n_byte = lrint (ceil (G->header->n_columns / 32.0)) * R->size;
	else if (fmt[0] == 'r' && fmt[1] == 'b')	/* Sun Raster uses multiple of 2 bytes */
		R->n_byte = lrint (ceil (G->header->n_columns / 2.0)) * 2 * R->size;
	else	/* All other */
		R->n_byte = G->header->n_columns * R->size;

	if (alloc) R->v_row = gmt_M_memory (GMT, NULL, R->n_byte, char);

	R->row = 0;
	R->auto_advance = (access_mode & GMT_GRID_ROW_BY_ROW_MANUAL) ? false : true;	/* Read sequentially or random-access rows */
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL void api_update_txt_item (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, size_t length, char string[]) {
	/* Place desired text in string (fixed size array) which can hold up to length bytes */
	size_t lim;
	static char buffer[GMT_BUFSIZ];
	char *txt = (mode & GMT_COMMENT_IS_OPTION) ? GMT_Create_Cmd (API, arg) : (char *)arg;
	gmt_M_memset (buffer, GMT_BUFSIZ, char);	/* Start with a clean slate */
	if ((mode & GMT_COMMENT_IS_OPTION) == 0 && (mode & GMT_COMMENT_IS_RESET) == 0 && string[0])
		strncat (buffer, string, length-1);	/* Use old text if we are not resetting */
	lim = length - strlen (buffer) - 1;	/* Remaining characters that we can use */
	if (mode & GMT_COMMENT_IS_OPTION) {	/* Must start with module name since it is not part of the option args */
		strncat (buffer, API->GMT->init.module_name, lim);
		lim = length - strlen (buffer) - 1;	/* Remaining characters that we can use */
		strncat (buffer, " ", lim);
	}
	lim = length - strlen (buffer) - 1;	/* Remaining characters that we can use */
	strncat (buffer, txt, lim);		/* Append new text */
	gmt_M_memset (string, length, char);	/* Wipe string completely */
	strncpy (string, buffer, length);	/* Only copy over max length bytes */
	if (mode & GMT_COMMENT_IS_OPTION) gmt_M_free (API->GMT, txt);
}

/*! . */
GMT_LOCAL void api_GI_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_GRID_HEADER *H) {
	/* Replace or Append either command or remark field with text or commmand-line options */
	if (mode & GMT_COMMENT_IS_REMARK) 	api_update_txt_item (API, mode, arg, GMT_GRID_REMARK_LEN160,  H->remark);
	else if (mode & GMT_COMMENT_IS_COMMAND) api_update_txt_item (API, mode, arg, GMT_GRID_COMMAND_LEN320, H->command);
	else if (mode & GMT_COMMENT_IS_TITLE)   api_update_txt_item (API, mode, arg, GMT_GRID_TITLE_LEN80,    H->title);
	else if (mode & GMT_COMMENT_IS_NAME_X)  api_update_txt_item (API, mode, arg, GMT_GRID_UNIT_LEN80,     H->x_units);
	else if (mode & GMT_COMMENT_IS_NAME_Y)  api_update_txt_item (API, mode, arg, GMT_GRID_UNIT_LEN80,     H->y_units);
	else if (mode & GMT_COMMENT_IS_NAME_Z)  api_update_txt_item (API, mode, arg, GMT_GRID_UNIT_LEN80,     H->z_units);
}

/*! Replace or Append either command or remark field with text or commmand-line options */
GMT_LOCAL void api_grid_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_GRID *G) {
	api_GI_comment (API, mode, arg, G->header);
}

/*! Update either command or remark field with text or commmand-line options */
GMT_LOCAL void api_image_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_IMAGE *I) {
	api_GI_comment (API, mode, arg, I->header);
}

/*! Update either command or remark field with text or commmand-line options */
GMT_LOCAL void api_vector_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_VECTOR *V) {
	if (mode & GMT_COMMENT_IS_REMARK)  api_update_txt_item (API, mode, arg, GMT_GRID_REMARK_LEN160,  V->remark);
	if (mode & GMT_COMMENT_IS_COMMAND) api_update_txt_item (API, mode, arg, GMT_GRID_COMMAND_LEN320, V->command);
}

/*! Update either command or remark field with text or commmand-line options */
GMT_LOCAL void api_matrix_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_MATRIX *M) {
	if (mode & GMT_COMMENT_IS_REMARK)  api_update_txt_item (API, mode, arg, GMT_GRID_REMARK_LEN160,  M->remark);
	if (mode & GMT_COMMENT_IS_COMMAND) api_update_txt_item (API, mode, arg, GMT_GRID_COMMAND_LEN320, M->command);
}

/*! Update common.h's various text items; return 1 if successful else 0 */
GMT_LOCAL int api_add_comment (struct GMTAPI_CTRL *API, unsigned int mode, char *txt) {
	unsigned int k = 0;
	struct GMT_COMMON *C = &API->GMT->common;	/* Short-hand to the common arg structs */

	if (mode & GMT_COMMENT_IS_TITLE)  { gmt_M_str_free (C->h.title); C->h.title = strdup (txt); k++; }
	if (mode & GMT_COMMENT_IS_REMARK) { gmt_M_str_free (C->h.remark); C->h.remark = strdup (txt); k++; }
	if (mode & GMT_COMMENT_IS_COLNAMES) { gmt_M_str_free (C->h.colnames); C->h.colnames = strdup (txt); k++; }
	return (k);	/* 1 if we did any of the three above; 0 otherwise */
}

/*! Append or replace data table headers with given text or commmand-line options */
GMT_LOCAL void api_dataset_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_DATASET *D) {
	unsigned int tbl, k;
	struct GMT_DATATABLE *T = NULL;
	char *txt = gmtapi_create_header_item (API, mode, arg);

	if (api_add_comment (API, mode, txt)) return;	/* Updated one -h item, or nothing */

	if (D->table == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "api_dataset_comment: Trying to access an empty D->table object\n");
		return;
	}

	/* Here we process free-form comments; these go into the dataset's header structures */
	for (tbl = 0; tbl < D->n_tables; tbl++) {	/* For each table in the dataset */
		T = D->table[tbl];	/* Short-hand for this table */
		if (mode & GMT_COMMENT_IS_RESET) {	/* Eliminate all existing headers */
			for (k = 0; k < T->n_headers; k++) gmt_M_str_free (T->header[k]);
			T->n_headers = 0;
		}
		T->header = gmt_M_memory (API->GMT, T->header, T->n_headers + 1, char *);
		T->header[T->n_headers++] = strdup (txt);
	}
}

/*! Append or replace text table headers with given text or commmand-line options */
GMT_LOCAL void api_textset_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_TEXTSET *D) {
	unsigned int tbl, k;
	struct GMT_TEXTTABLE *T = NULL;
	char *txt = gmtapi_create_header_item (API, mode, arg);

	if (api_add_comment (API, mode, txt)) return;	/* Updated one -h item or nothing */

	if (D->table == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "api_textset_comment: Trying to access an empty D->table object\n");
		return;
	}

	/* Here we process free-form comments; these go into the textset's header structures */
	for (tbl = 0; tbl < D->n_tables; tbl++) {	/* For each table in the dataset */
		T = D->table[tbl];	/* Short-hand for this table */
		if (mode & GMT_COMMENT_IS_RESET) {	/* Eliminate all existing headers */
			for (k = 0; k < T->n_headers; k++) gmt_M_str_free (T->header[k]);
			T->n_headers = 0;
		}
		T->header = gmt_M_memory (API->GMT, T->header, T->n_headers + 1, char *);
		T->header[T->n_headers++] = strdup (txt);
	}
}

/*! Append or replace CPT headers with given text or commmand-line options */
GMT_LOCAL void api_cpt_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_PALETTE *P) {
	unsigned int k;
	char *txt = gmtapi_create_header_item (API, mode, arg);

	if (!api_add_comment (API, mode, txt)) return;	/* Updated one -h item or nothing */

	/* Here we process free-form comments; these go into the CPT's header structures */
	if (mode & GMT_COMMENT_IS_RESET) {	/* Eliminate all existing headers */
		for (k = 0; k < P->n_headers; k++) gmt_M_str_free (P->header[k]);
		P->n_headers = 0;
	}
	P->header = gmt_M_memory (API->GMT, P->header, P->n_headers + 1, char *);
	P->header[P->n_headers++] = strdup (txt);
}

/*! Append or replace Postscript container headers with given text or commmand-line options */
GMT_LOCAL void api_ps_comment (struct GMTAPI_CTRL *API, unsigned int mode, void *arg, struct GMT_POSTSCRIPT *P) {
	unsigned int k;
	char *txt = gmtapi_create_header_item (API, mode, arg);

	if (!api_add_comment (API, mode, txt)) return;	/* Updated one -h item or nothing */

	/* Here we process free-form comments; these go into the CPT's header structures */
	if (mode & GMT_COMMENT_IS_RESET) {	/* Eliminate all existing headers */
		for (k = 0; k < P->n_headers; k++) gmt_M_str_free (P->header[k]);
		P->n_headers = 0;
	}
	P->header = gmt_M_memory (API->GMT, P->header, P->n_headers + 1, char *);
	P->header[P->n_headers++] = strdup (txt);
}

GMT_LOCAL unsigned int api_set_method (struct GMTAPI_DATA_OBJECT *S) {
	/* Most objects have a one-to-one path but for vectors and matrices
	 * we need to set the bit that correspond to their type */
	unsigned int m;
	if (S->method < GMT_IS_DUPLICATE) return S->method;
	switch (S->actual_family) {
		case GMT_IS_VECTOR: m = S->method | GMT_VIA_VECTOR; break;
		case GMT_IS_MATRIX: m = S->method | GMT_VIA_MATRIX; break;
		default: m = S->method;
	}
	return m;
}

/*! . */
GMT_LOCAL int api_next_io_source (struct GMTAPI_CTRL *API, unsigned int direction) {
	/* Get ready for the next source/destination (open file, initialize counters, etc.).
	 * Note this is only a mechanism for dataset and textset files where it is common
	 * to give many files on the command line (e.g., *.txt) and we do rec-by-rec processing.
	 * Not used by modules who read entire datasets in one go via GMT_{Read|Write}_Data,
	 * such as grids, images, palettes, postscript, but also datasets and texsets when
	 * GMT_Read_Data are used.  This section is strictly related to GMT_Get_Record. */

	int *fd = NULL;	/* !!! This MUST be int* due to nature of UNIX system function */
	unsigned int method, kind, first = 0;
	static const char *dir[2] = {"from", "to"};
	static const char *operation[3] = {"Reading", "Writing", "Appending"};
	char *mode = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	S_obj = API->object[API->current_item[direction]];	/* For shorthand purposes only */
	GMT_Report (API, GMT_MSG_DEBUG, "api_next_io_source: Selected object %d\n", S_obj->ID);
    gmt_M_memset (GMT->current.io.curr_pos[direction], 4U, int64_t);	/* Reset file, seg, point, header counters */
    if (direction == GMT_IN) {	/* Set reading mode */
		mode = GMT->current.io.r_mode;
		GMT->current.io.curr_pos[GMT_IN][GMT_SEG] = -1;	/* First segment of input is set to -1 until first segment header have been dealt with */
	}
	else	/* Set writing mode (but could be changed to append if GMT_IS_FILE and filename starts with >) */
		mode = GMT->current.io.w_mode;
	S_obj->close_file = false;	/* Do not want to close file pointers passed to us unless WE open them below */
	/* Either use binary n_columns settings or initialize to unknown if ascii input, i.e., GMT_MAX_COLUMNS */
	S_obj->n_expected_fields = (GMT->common.b.ncol[direction]) ? GMT->common.b.ncol[direction] : GMT_MAX_COLUMNS;

	method = api_set_method (S_obj);	/* Get the actual method to use since may be MATRIX or VECTOR masquerading as DATASET */
	switch (method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:	/* Filename given; we must open the file here */
			assert (S_obj->filename != NULL);
			if (S_obj->family == GMT_IS_GRID || S_obj->family == GMT_IS_IMAGE) return (gmtapi_report_error (API, GMT_NOT_A_VALID_TYPE));	/* Grids or images not allowed here */
			first = gmt_download_file_if_not_found (API->GMT, S_obj->filename);	/* Deal with downloadable GMT data sets first */
			if (direction == GMT_OUT && S_obj->filename[0] == '>') {
				mode = GMT->current.io.a_mode;	/* Must append to an existing file (we have already checked the file exists) */
				first = 1;
			}
			if ((S_obj->fp = gmt_fopen (GMT, &(S_obj->filename[first]), mode)) == NULL) {	/* Trouble opening file */
				GMT_Report (API, GMT_MSG_NORMAL, "Unable to open file %s for %s\n", &(S_obj->filename[first]), GMT_direction[direction]);
				return (GMT_ERROR_ON_FOPEN);
			}
			S_obj->close_file = true;	/* We do want to close files we are opening, but later */
			strncpy (GMT->current.io.filename[direction], &(S_obj->filename[first]), GMT_BUFSIZ-1);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s file %s\n",
				operation[direction+first], GMT_family[S_obj->family], dir[direction], &(S_obj->filename[first]));
			if (gmt_M_binary_header (GMT, direction)) {
				gmtlib_io_binary_header (GMT, S_obj->fp, direction);
				GMT_Report (API, GMT_MSG_NORMAL, "%s %d bytes of header %s binary file %s\n",
					operation[direction], GMT->current.setting.io_n_header_items, dir[direction], &(S_obj->filename[first]));
			}
			break;

		case GMT_IS_STREAM:	/* Given a stream; no need to open (or close) anything */
#ifdef SET_IO_MODE
			if (S_obj->family == GMT_IS_DATASET && S_obj->fp == GMT->session.std[direction])
				gmt_setmode (GMT, (int)direction);	/* Windows may need to have its read mode changed from text to binary */
#endif
			kind = (S_obj->fp == GMT->session.std[direction]) ? 0 : 1;	/* For message only: 0 if stdin/out, 1 otherwise for user pointer */
			snprintf (GMT->current.io.filename[direction], GMT_BUFSIZ, "<%s %s>", GMT_stream[kind], GMT_direction[direction]);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s %s %s stream\n",
				operation[direction], GMT_family[S_obj->family], dir[direction], GMT_stream[kind], GMT_direction[direction]);
			if (gmt_M_binary_header (GMT, direction)) {
				gmtlib_io_binary_header (GMT, S_obj->fp, direction);
				GMT_Report (API, GMT_MSG_NORMAL, "%s %d bytes of header %s binary %s stream\n",
					operation[direction], GMT->current.setting.io_n_header_items, dir[direction], GMT_stream[kind]);
			}
			break;

		case GMT_IS_FDESC:	/* Given a pointer to a file handle; otherwise same as stream */
			fd = (int *)S_obj->fp;	/* Extract the file handle integer */
			if ((S_obj->fp = fdopen (*fd, mode)) == NULL) {	/* Reopen handle as stream */
				GMT_Report (API, GMT_MSG_NORMAL, "Unable to open file descriptor %d for %s\n", *fd, GMT_direction[direction]);
				return (GMT_ERROR_ON_FDOPEN);
			}
			S_obj->method = S_obj->method - GMT_IS_FDESC + GMT_IS_STREAM;	/* Since fp now holds stream pointer an we have lost the handle */
			kind = (S_obj->fp == GMT->session.std[direction]) ? 0 : 1;	/* For message only: 0 if stdin/out, 1 otherwise for user pointer */
			snprintf (GMT->current.io.filename[direction], GMT_BUFSIZ, "<%s %s>", GMT_stream[kind], GMT_direction[direction]);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s %s %s stream via supplied file descriptor\n",
				operation[direction], GMT_family[S_obj->family], dir[direction], GMT_stream[kind], GMT_direction[direction]);
			if (gmt_M_binary_header (GMT, direction)) {
				gmtlib_io_binary_header (GMT, S_obj->fp, direction);
				GMT_Report (API, GMT_MSG_NORMAL, "%s %d bytes of header %s binary %s stream via supplied file descriptor\n",
					operation[direction], GMT->current.setting.io_n_header_items, dir[direction], GMT_stream[kind]);
			}
			break;

	 	case GMT_IS_DUPLICATE:	/* Copy, nothing to do [PW: not tested] */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s memory copy supplied by pointer\n",
				operation[direction], GMT_family[S_obj->family], dir[direction]);
			break;

	 	case GMT_IS_REFERENCE:	/* Reference, nothing to do [PW: not tested] */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s memory reference supplied by pointer\n",
				operation[direction], GMT_family[S_obj->family], dir[direction]);
			break;

	 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* These 2 mean reading or writing a dataset record-by-record via a user matrix */
		case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
			if (S_obj->family != GMT_IS_DATASET) return (gmtapi_report_error (API, GMT_NOT_A_VALID_TYPE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s %s memory location via matrix\n",
				operation[direction], GMT_family[S_obj->family], dir[direction], GMT_direction[direction]);
			if (direction == GMT_IN) {	/* Hard-wired limits are passed in from calling program; for outout we have nothing yet */
				if ((M_obj = S_obj->resource) == NULL) {
					GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: api_next_io_source got a matrix pointer that is NULL!!!\n");
					return GMT_OK;
				}
				S_obj->n_rows    = M_obj->n_rows;
				S_obj->n_columns = M_obj->n_columns;
				S_obj->rec = 0;	/* Start of this "file" */
				GMT->common.b.ncol[direction] = M_obj->n_columns;	/* Basically doing binary i/o with specified number of columns */
			}
			GMT->common.b.active[direction] = true;	/* Basically, we are doing what GMT calls binary i/o since it is all in memory */
			strcpy (GMT->current.io.filename[direction], "<matrix memory>");
			break;

		 case GMT_IS_DUPLICATE|GMT_VIA_VECTOR:	/* These 2 mean reading or writing a dataset record-by-record via user vector arrays */
		 case GMT_IS_REFERENCE|GMT_VIA_VECTOR:
			if (S_obj->family != GMT_IS_DATASET) return (gmtapi_report_error (API, GMT_NOT_A_VALID_TYPE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "%s %s %s %s memory location via vector\n",
					operation[direction], GMT_family[S_obj->family], dir[direction], GMT_direction[direction]);
			if (direction == GMT_IN) {	/* Hard-wired limits are passed in from calling program; for outout we have nothing yet */
				if ((V_obj = S_obj->resource) == NULL) {
					GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: api_next_io_source got a vector pointer that is NULL!!!\n");
					return GMT_OK;
				}
				S_obj->n_rows    = V_obj->n_rows;
				S_obj->n_columns = V_obj->n_columns;
				S_obj->rec = 0;	/* Start of this "file" */
				GMT->common.b.ncol[direction] = V_obj->n_columns;	/* Basically doing binary i/o with specified number of columns */
			}
			GMT->common.b.active[direction] = true;	/* Basically, we are doing what GMT calls binary i/o */
			strcpy (GMT->current.io.filename[direction], "<vector memory>");
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: api_next_io_source called with illegal method\n");
			break;
	}

	/* A few things pertaining only to data/text tables */
	GMT->current.io.rec_in_tbl_no = 0;	/* Start on new table */
	S_obj->import = (S_obj->family == GMT_IS_TEXTSET) ? &gmtio_ascii_textinput : GMT->current.io.input;	/* The latter may point to ASCII or binary input functions */

	return (GMT_OK);
}

/*! . */
GMT_LOCAL int api_next_data_object (struct GMTAPI_CTRL *API, enum GMT_enum_family family, enum GMT_enum_std direction) {
	/* Sets up current_item to be the next unused item of the required direction; or return EOF.
	 * When EOF is returned, API->current_item[direction] holds the last object ID used. */
	bool found = false;
	int item = API->current_item[direction] + 1;	/* Advance to next item, if it exists */
	while (item < (int)API->n_objects && !found) {
		if (API->object[item] && API->object[item]->selected && API->object[item]->status == GMT_IS_UNUSED
		    && API->object[item]->direction == direction && family == API->object[item]->family)
			found = true;	/* Got item that is selected and unused, has correct direction and family */
		else
			item++;	/* No, keep looking */
	}
	if (found) {	/* Update to use next item */
		API->current_item[direction] = item;	/* The next item */
		return (api_next_io_source (API, direction));	/* Initialize the next source/destination */
	}
	else
		return (EOF);	/* No more objects available for this direction; return EOF */
}

/*! Hook object to end of linked list and assign unique id (> 0) which is returned */
GMT_LOCAL int api_add_data_object (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *object) {
	/* Find the first entry in the API->object array which is unoccupied, and if
	 * they are all occupied then reallocate the array to make more space.
	 * We thus find and return the lowest available ID. */
	API->n_objects++;		/* Must add one more entry to the tally */
	if (API->n_objects == API->n_objects_alloc) {	/* Must allocate more space to hold all data descriptors */
		size_t old_n_alloc = API->n_objects_alloc;
		API->n_objects_alloc <<= 1;	/* Double it */
		API->object = gmt_M_memory (API->GMT, API->object, API->n_objects_alloc, struct GMTAPI_DATA_OBJECT *);
		if (!(API->object)) {		/* Failed to allocate more memory */
			API->n_objects--;	/* Undo our premature increment */
			return_value (API, GMT_MEMORY_ERROR, GMT_NOTSET);
		}
		else	/* Set new ones to NULL */
			gmt_M_memset (&(API->object[old_n_alloc]), API->n_objects_alloc - old_n_alloc, struct GMTAPI_DATA_OBJECT *);
	}
	object->ID = API->unique_ID++;	/* Assign a unique object ID */
	API->object[API->n_objects-1] = object;		/* Hook the current object onto the end of the list */

	return (object->ID);
}

/*! Sanity check that geometry and family are compatible; note they may not be set (GMT_NOTSET) hence the use of signed ints */
GMT_LOCAL bool api_validate_geometry (struct GMTAPI_CTRL *API, int family, int geometry) {
	bool problem = false;
	gmt_M_unused(API);
	if (geometry == GMT_NOTSET || family == GMT_NOTSET) return false;	/* No errors if nothing to check yet */
	switch (family) {
		case GMT_IS_TEXTSET:     if (!(geometry == GMT_IS_NONE || (geometry & GMT_IS_PLP))) problem = true; break;	/* Textsets can hold many things... */
		case GMT_IS_DATASET:     if (!(geometry == GMT_IS_NONE || (geometry & GMT_IS_PLP))) problem = true; break;	/* Datasets can hold many things... */
		case GMT_IS_GRID:        if (geometry != GMT_IS_SURFACE) problem = true;    break;	/* Only surface is valid */
		case GMT_IS_IMAGE:       if (geometry != GMT_IS_SURFACE) problem = true;    break;	/* Only surface is valid */
		case GMT_IS_PALETTE:     if (geometry != GMT_IS_NONE) problem = true;       break;	/* Only text is valid */
		case GMT_IS_POSTSCRIPT:  if (geometry != GMT_IS_NONE) problem = true;       break;	/* Only text is valid */
		case GMT_IS_VECTOR:      if ((geometry & GMT_IS_PLP) == 0) problem = true;  break; 	/* Must be one of those three */
		case GMT_IS_MATRIX:      if (geometry == GMT_IS_NONE) problem = true;       break;	/* Matrix can hold surfaces or TEXTSETs */
		case GMT_IS_COORD:       if (geometry != GMT_IS_NONE) problem = true;       break;	/* Only text is valid */
	}
	return (problem);
}

/*! . */
GMT_LOCAL int api_decode_id (const char *filename) {
	/* Checking if filename contains a name with embedded GMTAPI Object ID.
	 * If found we return the ID, otherwise we return GMT_NOTSET.
 	*/
	int object_ID = GMT_NOTSET;

	if (gmt_M_file_is_memory (filename)) {	/* Passing ID of a registered object */
		if (sscanf (&filename[9], "%d", &object_ID) != 1) return (GMT_NOTSET);	/* Get the object ID unless we fail scanning */
	}
	return (object_ID);	/* Returns GMT_NOTSET if no embedded ID was found */
}

/*! . */
GMT_LOCAL int api_get_object (struct GMTAPI_CTRL *API, int sfamily, void *ptr) {
	/* Returns the ID of the first object whose data pointer matches ptr.
	 * Unless family is GMT_NOTSET the object must be of the specified family.
	 */
	unsigned int item;
	enum GMT_enum_family family = GMT_NOTSET;
	int object_ID = GMT_NOTSET;	/* Not found yet */

	if (sfamily != GMT_NOTSET) family = sfamily;
	for (item = 0; object_ID == GMT_NOTSET && item < API->n_objects; item++) {	/* Loop over all objects */
		if (!API->object[item]) continue;	/* Skip freed objects */
		if (API->object[item]->data == NULL) continue;	/* No data pointer */
		if (sfamily != GMT_NOTSET && API->object[item]->family != family) continue;	/* Not the right family */
		if (API->object[item]->data == ptr && object_ID == GMT_NOTSET) object_ID = API->object[item]->ID;	/* Found a matching data pointer */
	}
	return (object_ID);	/* Return ID or GMT_NOTSET if not found */
}

/*! . */
GMT_LOCAL void *api_pass_object (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *object, unsigned int family, double *wesn) {
	/* Passes back the input object pointer after possibly performing some minor adjustments to metadata.
	 * For grids and images we must worry about possible subset requests */
	void *data = (object->data) ? object->data : object->resource;	/* Get pointer to the data TEMPORARY: SHOULD BE data, NO? */
	struct GMT_GRID    *G = NULL;
	struct GMT_IMAGE   *I = NULL;
	struct GMT_DATASET *D = NULL;

	if (object->resource && object->data && (object->resource != object->data))
		GMT_Report (API, GMT_MSG_NORMAL, "ERROR?: api_pass_object given resource = %" PRIxS " and data = %" PRIxS " and cannot choose sensibly: data was selected\n", object->resource, object->data);

	switch (family) {	/* Do family-specific prepping before passing back the input object */
		case GMT_IS_PALETTE:	/* Make sure the hidden support arrays etc. have been initialized as external interfaces may not care */
			if (data) gmtlib_init_cpt (API->GMT, data);
			break;
		case GMT_IS_GRID:	/* Grids need to update the grdtype setting, possibly rotate geographic grids, and maybe deal with subsets */
			G = api_get_grid_data (data);	/* Get the right grid pointer */
			gmtlib_grd_get_units (API->GMT, G->header);	/* Set the unit names */
			G->header->grdtype = gmtlib_get_grdtype (API->GMT, G->header);
			if (wesn && G->data) {	/* Subset or global rotation was requested */
				if (gmt_M_grd_is_global (API->GMT, G->header)) {	/* May have to rotate a geographic grid since we are not reading from file this time */
					double shift_amount = wesn[XLO] - G->header->wesn[XLO];
					if (fabs (shift_amount) >= G->header->inc[GMT_X]) {	/* Must do it */
						GMT_Report (API, GMT_MSG_DEBUG, "Shifting longitudes in grid by %g degrees to fit -R\n", shift_amount);
						gmt_grd_shift (API->GMT, G, shift_amount);	/* In-memory rotation */
					}
				}
				else if (object->region) {	/* Possibly adjust the pad so inner region matches requested wesn */
					/* NOTE: This assumes the memory cannot be adjusted. Probably should distinaguish between GMT_IS_REFERENCE and GMT_IS_DUPLICATE
					 * and offer different behavior.  As it is we assume read-only grids */
					if (object->reset_pad) {	/* First undo any prior sub-region used with this memory grid */
						gmtlib_contract_headerpad (API->GMT, G->header, object->orig_pad, object->orig_wesn);
						object->reset_pad = G->header->reset_pad = 0;
					}
					/* Then apply the new pad adjustment.  Basically we cannot mess with the data so we change what constitute the pad */
					if (gmtlib_expand_headerpad (API->GMT, G->header, object->wesn, object->orig_pad, object->orig_wesn))
						object->reset_pad = G->header->reset_pad = 1;
				}
			}
			break;
		case GMT_IS_IMAGE:	/* Images need to update the grdtype setting, possibly rotate geographic grids, and maybe deal with subsets */
			I = api_get_image_data (data);	/* Get the right image pointer */
			gmtlib_grd_get_units (API->GMT, I->header);	/* Set the unit names */
			I->header->grdtype = gmtlib_get_grdtype (API->GMT, I->header);
			if (wesn && I->data) {	/* Subset or global rotation was requested */
				if (gmt_M_grd_is_global (API->GMT, I->header)) {	/* May have to rotate geographic grid since we are not reading from file here */
					double shift_amount = wesn[XLO] - I->header->wesn[XLO];
					if (fabs (shift_amount) >= I->header->inc[GMT_X]) {	/* Must do it */
						GMT_Report (API, GMT_MSG_NORMAL, "Longitudinal roll for images not implemented yet\n");
#if 0
						GMT_Report (API, GMT_MSG_DEBUG, "Shifting longitudes in grid by %g degrees to fit -R\n", shift_amount);
						gmt_grd_shift (API->GMT, I, shift_amount);
#endif
					}
				}
				else if (object->region) {	/* Possibly adjust the pad so inner region matches wesn */
					/* NOTE: This assumes the memory cannot be adjusted. Probably should distinaguish between GMT_IS_REFERENCE and GMT_IS_DUPLICATE
					 * and offer different behavior.  As it is we assume read-only images */
					if (object->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
						gmtlib_contract_headerpad (API->GMT, I->header, object->orig_pad, object->orig_wesn);
						object->reset_pad = I->header->reset_pad = 0;
					}
					/* Then apply the new pad adjustment.  Basically we cannot mess with the data so we change what constitute the pad */
					if (gmtlib_expand_headerpad (API->GMT, I->header, object->wesn, object->orig_pad, object->orig_wesn))
						object->reset_pad = I->header->reset_pad = 1;
				}
			}
			break;
		case GMT_IS_DATASET:	/* Just make sure the min/max values are updated for tables and dataset  */
		 	D = api_get_dataset_data (data);	/* Get the right dataset pointer */
			gmtlib_set_dataset_minmax (API->GMT, D);	/* Set the min/max values for the entire dataset */
			break;
		default:	/* Nothing yet for other types */
			break;
	}
	return (data);
}

/*! . */
GMT_LOCAL int api_get_object_id_from_data_ptr (struct GMTAPI_CTRL *API, void *ptr) {
	/* Returns the ID of the first object whose data pointer matches *ptr.
 	 * This is necessary since many objects may have the same pointer
	 * but we only want to destroy the memory once.  This function is
	 * only used in GMT_Destroy_Data and gmtlib_is_an_object.
	 */
	unsigned int item;
	int object_ID = GMT_NOTSET;	/* Not found yet */
	void *data = NULL;

	for (item = 0; object_ID == GMT_NOTSET && item < API->n_objects; item++) {	/* Loop over all objects */
		if (!API->object[item]) continue;	/* Skip freed objects */
		data = api_return_address (ptr, API->object[item]->family);	/* Get void* pointer to resource of this family */
		//if (API->object[item]->data == data && object_ID == GMT_NOTSET) object_ID = API->object[item]->ID;	/* Found a matching data pointer */
		/* Try to look for either data or resource pointers since Open_VirtualFile shuffles these two and Destroy needs to find them even if the
		 * function level test will tell it not to free anything */
		if (object_ID == GMT_NOTSET && (API->object[item]->data == data || API->object[item]->resource == data)) object_ID = API->object[item]->ID;	/* Found a matching data pointer */
	}
	return (object_ID);	/* Return ID or GMT_NOTSET if not found */
}

/*! . */
bool gmtlib_is_an_object (struct GMT_CTRL *GMT, void *ptr) {
	/* Needed by g*math.c so exported as part of the gmt_dev library */
	return (api_get_object_id_from_data_ptr (GMT->parent, ptr) == GMT_NOTSET) ? false : true;
}

/*! . */
GMT_LOCAL int api_is_registered (struct GMTAPI_CTRL *API, enum GMT_enum_family family, enum GMT_enum_geometry geometry, enum GMT_enum_std direction, unsigned int mode, char *filename, void *resource) {
	/* Checks to see if the given data pointer has already been registered.
 	 * This can happen for grids which first gets registered reading the header
 	 * and then is registered again when reading the whole grid.  In those cases
	 * we don't want to register them twice.
	 */
	unsigned int i;
	int item;

	if (API->n_objects == 0) return (GMT_NOTSET);	/* There are no known resources yet */

	 /* Search for the object in the active list.  However, if object_ID == GMT_NOTSET we instead pick the first in that direction */

	for (i = 0, item = GMT_NOTSET; item == GMT_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;	/* Skip empty objects */
		if (API->object[i]->status != GMT_IS_UNUSED) {	/* Has already been read - do we wish to reset this status ? */
			if (family == GMT_IS_GRID && (mode & GMT_DATA_ONLY)) {	/* Requesting data only means we already did the header so OK to reset status */
				if (mode & GMT_GRID_IS_COMPLEX_MASK) {	/* Complex grids are read in stages so handled separately */
					/* Check if complex grid already has one layer and that we are reading the next layer */
					struct GMT_GRID *G = api_get_grid_data (resource);	/* Get pointer to this grid */
					unsigned int cmplx = mode & GMT_GRID_IS_COMPLEX_MASK;
					if (G->header->complex_mode & GMT_GRID_IS_COMPLEX_MASK && G->header->complex_mode != cmplx && filename) {
						/* Apparently so, either had real and now getting imag, or vice versa. */
						gmt_M_str_free (API->object[i]->filename);	/* Free previous grid name and replace with current name */
						API->object[i]->filename = strdup (filename);
						mode |= GMT_IO_RESET;	/* Reset so we may read in the 2nd component grid */
					}
				}
				else	/* Just read the header earlier, do the reset */
					mode |= GMT_IO_RESET;	/* Reset so we may read in the grid data */
			}
			else if (family == GMT_IS_IMAGE && (mode & GMT_DATA_ONLY))	/* Requesting data only means we already did the header so OK to reset status */
				mode |= GMT_IO_RESET;	/* Reset so we may read in the image data */

			if (!(mode & GMT_IO_RESET)) continue;	/* No reset above so we refuse to do the work */
			API->object[i]->status = GMT_IS_UNUSED;	/* Reset so we may continue to read it */
		}
		if (API->object[i]->direction != direction) continue;	/* Wrong direction */
		if (API->object[i]->family != family) continue;		/* Wrong family */
		if (API->object[i]->geometry != geometry) continue;	/* Wrong geometry */
		if (resource && API->object[i]->resource == resource) item = API->object[i]->ID;	/* Yes: already registered NOTE PW: One of these may not be needed? */
		else if (resource && API->object[i]->data == resource) item = API->object[i]->ID;	/* Yes: already registered */
	}
	return (item);		/* The ID of the object (or GMT_NOTSET) */
}

/*! . */
GMT_LOCAL struct GMT_PALETTE * api_import_palette (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in a CPT palette table.
 	 * The mode controls how the back-, fore-, NaN-color entries are handled.
	 * Note: Memory is allocated to hold the GMT_PALETTE structure except for method GMT_IS_REFERENCE.
	 */

	int item;
	unsigned int flag = 0, kind;
	char tmp_cptfile[GMT_LEN64] = {""};
	struct GMT_PALETTE *P_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_palette: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return_null (API, GMT_NO_INPUT);	/* Need to know the ID to do anything */
	if ((item = gmtapi_validate_id (API, GMT_IS_PALETTE, object_ID, GMT_IN, GMTAPI_OPTION_INPUT)) == GMT_NOTSET)
		return_null (API, API->error);	/* Failed basic sanity check */

	S_obj = API->object[item];	/* Use S_obj as shorthand */
	if (S_obj->status != GMT_IS_UNUSED) { /* Already read this resource before; are we allowed to re-read? */
		if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC)
			return_null (API, GMT_READ_ONCE); /* Not allowed to re-read streams since they are gone */
		if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
	}

	/* OK, passed sanity and is allowed to read */

	switch (S_obj->method) {	/* From where are we getting the palette ? */
		case GMT_IS_FILE:
			/* gmtlib_read_cpt will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading CPT from %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			snprintf (tmp_cptfile, GMT_LEN64, "api_colors2cpt_%d.cpt", (int)getpid());
			if (!strcmp (tmp_cptfile, S_obj->filename))	/* This file was created when we gave "name" as red,blue,... instead */
			 	flag = GMT_CPT_TEMPORARY;	/* So we can take action later when we learn if user wanted a discrete or continuous CPT */
			if ((P_obj = gmtlib_read_cpt (GMT, S_obj->filename, S_obj->method, mode|flag)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			if (flag == GMT_CPT_TEMPORARY) {	/* Remove the temporary file */
				GMT_Report (API, GMT_MSG_DEBUG, "Remove temporary CPT %s\n", S_obj->filename);
				remove (tmp_cptfile);
			}
			break;
		case GMT_IS_STREAM:
 			/* gmtlib_read_cpt will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_IN]) ? 0 : 1;	/* 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading CPT from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((P_obj = gmtlib_read_cpt (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_FDESC:
			/* gmtlib_read_cpt will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_IN) ? 0 : 1;	/* 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading CPT from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((P_obj = gmtlib_read_cpt (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_DUPLICATE:	/* Duplicate the input CPT palette */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating CPT from GMT_PALETTE memory location\n");
			if (S_obj->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
			P_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_PALETTE);
			gmtlib_copy_palette (GMT, P_obj, S_obj->resource);
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location, so nothing is allocated */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing CPT from GMT_PALETTE memory location\n");
			if ((P_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			gmtlib_init_cpt (GMT, P_obj);	/* Make sure derived quantities are set */
			break;
		default:	/* Barking up the wrong tree here... */
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import GMT_PALETTE\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as read */
	S_obj->data = P_obj;		/* Retain pointer to the allocated data so we use garbage collection later */
#ifdef GMT_BACKWARDS_API
	P_obj->range = P_obj->data;
	P_obj->patch = P_obj->bfn;
	P_obj->cpt_flags = P_obj->mode;
#endif
	return (P_obj);	/* Pass back the palette */
}

/*! . */
GMT_LOCAL int api_export_palette (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_PALETTE *P_obj) {
	/* Does the actual work of writing out the specified CPT to a destination.
	 * The mode controls how the back, for, NaN color entries are handled.
	 */
	int item, error;
	unsigned int kind;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_PALETTE *P_copy = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_palette: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_PALETTE, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* This is the API object for the output destination */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) {	/* Only allow writing of a data set once, unless we override by resetting the mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	}
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;

	/* Passed sanity and allowed to write */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* gmtlib_write_cpt will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write CPT to %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((error = gmtlib_write_cpt (GMT, S_obj->filename, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_STREAM:
			/* gmtlib_write_cpt will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_OUT]) ? 0 : 1;	/* 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write CPT to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = gmtlib_write_cpt (GMT, S_obj->fp, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_FDESC:
			/* gmtlib_write_cpt will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_OUT) ? 0 : 1;	/* 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write CPT to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = gmtlib_write_cpt (GMT, S_obj->fp, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
		case GMT_IS_DUPLICATE:		/* Duplicate the input cpt */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating CPT to GMT_PALETTE memory location\n");
			P_copy = gmt_M_memory (GMT, NULL, 1, struct GMT_PALETTE);
			gmtlib_copy_palette (GMT, P_copy, P_obj);
			S_obj->resource = P_copy;	/* Set resource pointer from object to this palette */
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing CPT to GMT_PALETTE memory location\n");
			P_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			S_obj->resource = P_obj;	/* Set resource pointer from object to this palette */
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export CPTs\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as written */
	S_obj->data = NULL;

	return GMT_OK;
}

/*! . */
GMT_LOCAL struct GMT_POSTSCRIPT * api_import_postscript (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in a PS struct.
 	 * The mode is not used yet.
	 * Note: Memory is allocated to hold the GMT_POSTSCRIPT structure except for method GMT_IS_REFERENCE.
	 */

	int item;
	unsigned int kind;
	struct GMT_POSTSCRIPT *P_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_postscript: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return_null (API, GMT_NO_INPUT);
	if ((item = gmtapi_validate_id (API, GMT_IS_POSTSCRIPT, object_ID, GMT_IN, GMTAPI_OPTION_INPUT)) == GMT_NOTSET)
		return_null (API, API->error);

	S_obj = API->object[item];	/* Use S_obj as shorthand */
	if (S_obj->status != GMT_IS_UNUSED) { /* Already read this resource before; are we allowed to re-read? */
		if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC) return_null (API, GMT_READ_ONCE); /* Not allowed to re-read streams */
		if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
	}

	/* Passed sanity and allowed to read */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* gmtlib_read_ps will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading PS from %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((P_obj = gmtlib_read_ps (GMT, S_obj->filename, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_STREAM:
 			/* gmtlib_read_ps will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_IN]) ? 0 : 1;	/* 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading PS from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((P_obj = gmtlib_read_ps (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_FDESC:
			/* gmtlib_read_ps will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_IN) ? 0 : 1;	/* 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading PS from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((P_obj = gmtlib_read_ps (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_DUPLICATE:	/* Duplicate the input CPT palette */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating PS from GMT_POSTSCRIPT memory location\n");
			if (S_obj->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
			P_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_POSTSCRIPT);
			gmtlib_copy_ps (GMT, P_obj, S_obj->resource);
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location, so nothing is allocated */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing PS from GMT_POSTSCRIPT memory location\n");
			if ((P_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			break;
		default:	/* Barking up the wrong tree here... */
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import PS\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}
	S_obj->alloc_mode = P_obj->alloc_mode;
	S_obj->status = GMT_IS_USED;	/* Mark as read */
	S_obj->data = P_obj;		/* Retain pointer to the allocated data so we use garbage collection later */

	return (P_obj);	/* Pass back the PS */
}

/*! . */
GMT_LOCAL int api_export_postscript (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_POSTSCRIPT *P_obj) {
	/* Does the actual work of writing out the specified PS to a destination.
	 * The mode not used yet.
	 */
	int item, error;
	unsigned int kind;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_POSTSCRIPT *P_copy = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_postscript: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_POSTSCRIPT, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* This is the API object for the output destination */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) {	/* Only allow writing of a data set once, unless we override by resetting the mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	}
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;

	/* Passed sanity and allowed to write */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* gmtlib_write_ps will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write PS to %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((error = gmtlib_write_ps (GMT, S_obj->filename, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_STREAM:
			/* gmtlib_write_ps will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_OUT]) ? 0 : 1;	/* 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write PS to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = gmtlib_write_ps (GMT, S_obj->fp, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_FDESC:
			/* gmtlib_write_ps will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_OUT) ? 0 : 1;	/* 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write PS to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = gmtlib_write_ps (GMT, S_obj->fp, S_obj->method, mode, P_obj))) return (gmtapi_report_error (API, error));
			break;
		case GMT_IS_DUPLICATE:		/* Duplicate the input cpt */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating PS to GMT_POSTSCRIPT memory location\n");
			P_copy = gmt_M_memory (GMT, NULL, 1, struct GMT_POSTSCRIPT);
			gmtlib_copy_ps (GMT, P_copy, P_obj);
			S_obj->resource = P_copy;	/* Set resource pointer from object to this PS */
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing PS to GMT_POSTSCRIPT memory location\n");
			P_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			S_obj->resource = P_obj;	/* Set resource pointer from object to this PS */
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export PS\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as written */
	S_obj->data = NULL;	/* Since this is an output result */

	return GMT_OK;
}

GMT_LOCAL struct GMT_MATRIX *api_read_matrix (struct GMT_CTRL *GMT, void *source, unsigned int src_type, unsigned int mode) {
	/* We read the MATRIX from fp [or stdin].
	 * src_type can be GMT_IS_[FILE|STREAM|FDESC]
	 * Notes: mode is not used yet.  We only do ascii file for now - later need to deal with -b, if needed.
	 */

	bool close_file = false, first = true, add_first_segheader = false;
	int error = 0;
	uint64_t row = 0, col, ij, dim[4] = {0, 0, 0, GMT->current.setting.export_type};
	char M_file[GMT_BUFSIZ] = {""};
	char line[GMT_BUFSIZ] = {""};
	FILE *fp = NULL;
	struct GMT_MATRIX *M = NULL;
	GMT_putfunction api_put_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	gmt_M_unused(mode);

	if (src_type == GMT_IS_FILE && !source) src_type = GMT_IS_STREAM;	/* No filename given, default to stdin */

	if (src_type == GMT_IS_FILE) {	/* dest is a file name */
		strncpy (M_file, source, GMT_BUFSIZ-1);
		if ((fp = fopen (M_file, "r")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot open Matrix file %s\n", M_file);
			return_null (GMT->parent, GMT_ERROR_ON_FOPEN);
		}
		close_file = true;	/* We only close files we have opened here */
	}
	else if (src_type == GMT_IS_STREAM) {	/* Open file pointer given, just copy */
		fp = (FILE *)source;
		if (fp == NULL) fp = GMT->session.std[GMT_IN];	/* Default destination */
		if (fp == GMT->session.std[GMT_IN])
			strcpy (M_file, "<stdin>");
		else
			strcpy (M_file, "<input stream>");
	}
	else if (src_type == GMT_IS_FDESC) {		/* Open file descriptor given, just convert to file pointer */
		int *fd = source;
		if (fd && (fp = fdopen (*fd, "r")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot convert Matrix file descriptor %d to stream in api_read_matrix\n", *fd);
			return_null (GMT->parent, GMT_ERROR_ON_FDOPEN);
		}
		if (fd == NULL) fp = GMT->session.std[GMT_IN];	/* Default destination */
		if (fp == GMT->session.std[GMT_IN])
			strcpy (M_file, "<stdin>");
		else
			strcpy (M_file, "<input file descriptor>");
		close_file = true;	/* since fdopen allocates space */
	}
	else {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unrecognized source type %d in api_read_matrix\n", src_type);
		return_null (GMT->parent, GMT_NOT_A_VALID_METHOD);
	}
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "Read Matrix from %s\n", M_file);

	while (!error && fgets (line, GMT_BUFSIZ, fp)) {
		if (line[0] == '#') continue;	/* Just skip headers */
		if (line[0] == '>') {
			if (first) {	/* Have not allocated yet so just skip that row for now and deal with it later */
				first = false;
				add_first_segheader = true;
			}
			else {	/* Already allocated so place NaNs as segment header */
				gmt_prep_tmp_arrays (GMT, row, dim[0]);	/* Init or reallocate tmp vectors */
				for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][row] = GMT->session.d_NaN;
			}
		}
		else {	/* Regular data record */
			gmt_chop (line);	/* Remove linefeeds */
			dim[0] = gmtlib_conv_text2datarec (GMT, line, GMT_BUFSIZ, GMT->current.io.curr_rec);
			gmt_prep_tmp_arrays (GMT, row, dim[0]);	/* Init or reallocate tmp vectors */
			for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][row] = GMT->current.io.curr_rec[col];
		}
		row++;
	}
	/* Possibly restore the missing first segment header */
	if (add_first_segheader) for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][0] = GMT->session.d_NaN;
	dim[1] = row;	/* Allocate all vectors using current type setting in the defaults [GMT_DOUBLE] */
	if ((M = GMT_Create_Data (GMT->parent, GMT_IS_MATRIX, GMT_IS_POINT, 0, dim, NULL, NULL, 0, 0, NULL)) == NULL) {
		if (close_file) fclose (fp);
		return_null (GMT->parent, GMT_MEMORY_ERROR);
	}
	api_put_val = api_select_put_function (GMT->parent, M->type);	/* Get correct put function given data type */
	GMT_2D_to_index = api_get_2d_to_index (GMT->parent, M->shape, GMT_GRID_IS_REAL);	/* Get ij index function */
	for (col = 0; col < M->n_columns; col++) {
		for (row = 0; row < M->n_rows; row++) {
			ij = GMT_2D_to_index (row, col, M->dim);	/* Index into the user data matrix depends on layout (M->shape) */
			api_put_val (&(M->data), ij, GMT->hidden.mem_coord[col][row]);
		}
	}

	if (close_file) fclose (fp);
	return (M);
}

/*! . */
GMT_LOCAL struct GMT_MATRIX *api_import_matrix (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in a GMT vector table.
	 */

	int item;
	unsigned int kind;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_matrix: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return_null (API, GMT_NO_INPUT);
	if ((item = gmtapi_validate_id (API, GMT_IS_MATRIX, object_ID, GMT_IN, GMTAPI_OPTION_INPUT)) == GMT_NOTSET)
		return_null (API, API->error);

	S_obj = API->object[item];	/* Use S_obj as shorthand */
	if (S_obj->status != GMT_IS_UNUSED) { /* Already read this resource before; are we allowed to re-read? */
		if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC)
			return_null (API, GMT_READ_ONCE); /* Not allowed to re-read streams */
		if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
	}

	/* Passed sanity and allowed to read */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading MATRIX from %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((M_obj = api_read_matrix (GMT, S_obj->filename, S_obj->method, mode)) == NULL)
				return_null (API, GMT_DATA_READ_ERROR);
			break;
		case GMT_IS_STREAM:
 			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_IN]) ? 0 : 1;	/* Used for message: 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading MATRIX from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((M_obj = api_read_matrix (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_DATA_READ_ERROR);
			break;
		case GMT_IS_FDESC:
			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_IN) ? 0 : 1;	/* Used for message: 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading MATRIX from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((M_obj = api_read_matrix (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_DUPLICATE:	/* Duplicate the input MATRIX */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating MATRIX from MATRIX memory location [NOT IMPLEMENTED YET]\n");
			if (S_obj->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
			//M_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_VECTOR);
			//gmtlib_copy_vector (GMT, M_obj, S_obj->resource);
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location, so nothing is allocated */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing MATRIX from MATRIX memory location\n");
			if ((M_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			break;
		default:	/* Barking up the wrong tree here... */
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import MATRIX\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as read */
	S_obj->data = M_obj;		/* Retain pointer to the allocated data so we use garbage collection later */
	return (M_obj);	/* Pass back the vector */
}

GMT_LOCAL int api_write_matrix (struct GMT_CTRL *GMT, void *dest, unsigned int dest_type, unsigned int mode, struct GMT_MATRIX *M) {
	/* We write the MATRIX to fp [or stdout].
	 * dest_type can be GMT_IS_[FILE|STREAM|FDESC]
	 * mode is not used yet.
	 */

	bool close_file = false, append = false;
	uint64_t row, col, ij;
	char M_file[GMT_BUFSIZ] = {""};
	static char *msg1[2] = {"Writing", "Appending"};
	FILE *fp = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_getfunction api_get_val = NULL;
	gmt_M_unused(mode);

	if (dest_type == GMT_IS_FILE && !dest) dest_type = GMT_IS_STREAM;	/* No filename given, default to stdout */

	if (dest_type == GMT_IS_FILE) {	/* dest is a file name */
		static char *msg2[2] = {"create", "append to"};
		strncpy (M_file, dest, GMT_BUFSIZ-1);
		append = (M_file[0] == '>');	/* Want to append to existing file */
		if ((fp = fopen (&M_file[append], (append) ? "a" : "w")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot %s Matrix file %s\n", msg2[append], &M_file[append]);
			return (GMT_ERROR_ON_FOPEN);
		}
		close_file = true;	/* We only close files we have opened here */
	}
	else if (dest_type == GMT_IS_STREAM) {	/* Open file pointer given, just copy */
		fp = (FILE *)dest;
		if (fp == NULL) fp = GMT->session.std[GMT_OUT];	/* Default destination */
		if (fp == GMT->session.std[GMT_OUT])
			strcpy (M_file, "<stdout>");
		else
			strcpy (M_file, "<output stream>");
	}
	else if (dest_type == GMT_IS_FDESC) {		/* Open file descriptor given, just convert to file pointer */
		int *fd = dest;
		if (fd && (fp = fdopen (*fd, "w")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot convert Matrix file descriptor %d to stream in api_write_matrix\n", *fd);
			return (GMT_ERROR_ON_FDOPEN);
		}
		if (fd == NULL) fp = GMT->session.std[GMT_OUT];	/* Default destination */
		if (fp == GMT->session.std[GMT_OUT])
			strcpy (M_file, "<stdout>");
		else
			strcpy (M_file, "<output file descriptor>");
		close_file = true;	/* since fdopen allocates space */
	}
	else {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unrecognized source type %d in api_write_matrix\n", dest_type);
		return (GMT_NOT_A_VALID_METHOD);
	}
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "%s Matrix to %s\n", msg1[append], &M_file[append]);

	/* Start writing Matrix to fp */

	GMT_2D_to_index = api_get_2d_to_index (GMT->parent, M->shape, GMT_GRID_IS_REAL);
	api_get_val = api_select_get_function (GMT->parent, M->type);

	for (row = 0; row < M->n_rows; row++) {
		for (col = 0; col < M->n_columns; col++) {
			ij = GMT_2D_to_index (row, col, M->dim);	/* Index into the user data matrix depends on layout (M->shape) */
			api_get_val (&(M->data), ij, &(GMT->current.io.curr_rec[col]));
		}
		if (api_bin_input_memory (GMT, M->n_columns, M->n_columns) < 0)	/* Segment header found, finish the segment we worked on and goto next */
			gmt_write_segmentheader (GMT, fp, M->n_columns);
		else {	/* Format an ascii output record */
			fprintf (fp, GMT->current.setting.format_float_out, GMT->current.io.curr_rec[0]);
			for (col = 1; col < M->n_columns; col++) {
				fprintf (fp, "%s", GMT->current.setting.io_col_separator);
				fprintf (fp, GMT->current.setting.format_float_out, GMT->current.io.curr_rec[col]);
			}
			fprintf (fp, "\n");
		}
	}

	if (close_file) fclose (fp);
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_export_matrix (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_MATRIX *M_obj) {
	/* Does the actual work of writing out the specified Matrix to a destination.  Only FILE supported for testing.
	 * The mode not used yet.
	 */
	int item, error;
	unsigned int kind;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_matrix: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_MATRIX, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* This is the API object for the output destination */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) {	/* Only allow writing of a data set once, unless we override by resetting the mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	}
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;

	/* Passed sanity and allowed to write */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* api_write_matrix will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write MATRIX to %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((error = api_write_matrix (GMT, S_obj->filename, S_obj->method, mode, M_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_STREAM:
			/* api_write_matrix will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_OUT]) ? 0 : 1;	/* For message only: 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write MATRIX to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = api_write_matrix (GMT, S_obj->fp, S_obj->method, mode, M_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_FDESC:
			/* api_write_matrix will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_OUT) ? 0 : 1;	/* For message only: 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write MATRIX to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = api_write_matrix (GMT, S_obj->fp, S_obj->method, mode, M_obj))) return (gmtapi_report_error (API, error));
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export MATRIX\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as written */
	S_obj->data = NULL;

	return GMT_OK;
}

GMT_LOCAL int api_write_vector (struct GMT_CTRL *GMT, void *dest, unsigned int dest_type, unsigned int mode, struct GMT_VECTOR *V) {
	/* We write the VECTOR to fp [or stdout].
	 * dest_type can be GMT_IS_[FILE|STREAM|FDESC]
	 * mode is not used yet.
	 */

	bool close_file = false, append = false;
	uint64_t row, col;
	char V_file[GMT_BUFSIZ] = {""};
	static char *msg1[2] = {"Writing", "Appending"};
	FILE *fp = NULL;
	GMT_getfunction *api_get_val = NULL;
	gmt_M_unused(mode);

	if (V == NULL) {
		GMT_Report(GMT->parent, GMT_MSG_NORMAL, "GMTAPI: api_write_vector passed a NULL pointer *V\n");
		return GMT_NOTSET;
	}
	if (dest_type == GMT_IS_FILE && !dest) dest_type = GMT_IS_STREAM;	/* No filename given, default to stdout */

	if (dest_type == GMT_IS_FILE) {	/* dest is a file name */
		static char *msg2[2] = {"create", "append to"};
		strncpy (V_file, dest, GMT_BUFSIZ-1);
		append = (V_file[0] == '>');	/* Want to append to existing file */
		if ((fp = fopen (&V_file[append], (append) ? "a" : "w")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot %s Matrix file %s\n", msg2[append], &V_file[append]);
			return (GMT_ERROR_ON_FOPEN);
		}
		close_file = true;	/* We only close files we have opened here */
	}
	else if (dest_type == GMT_IS_STREAM) {	/* Open file pointer given, just copy */
		fp = (FILE *)dest;
		if (fp == NULL) fp = GMT->session.std[GMT_OUT];	/* Default destination */
		if (fp == GMT->session.std[GMT_OUT])
			strcpy (V_file, "<stdout>");
		else
			strcpy (V_file, "<output stream>");
	}
	else if (dest_type == GMT_IS_FDESC) {		/* Open file descriptor given, just convert to file pointer */
		int *fd = dest;
		if (fd && (fp = fdopen (*fd, "a")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot convert Matrix file descriptor %d to stream in api_write_matrix\n", *fd);
			return (GMT_ERROR_ON_FDOPEN);
		}
		if (fd == NULL) fp = GMT->session.std[GMT_OUT];	/* Default destination */
		if (fp == GMT->session.std[GMT_OUT])
			strcpy (V_file, "<stdout>");
		else
			strcpy (V_file, "<output file descriptor>");
		close_file = true;	/* since fdopen allocates space */
	}
	else {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unrecognized source type %d in api_write_matrix\n", dest_type);
		return (GMT_NOT_A_VALID_METHOD);
	}
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "%s Matrix to %s\n", msg1[append], &V_file[append]);

	/* Start writing Vector to fp */

	api_get_val = gmt_M_memory (GMT, NULL, V->n_columns, GMT_getfunction);
	for (col = 0; col < V->n_columns; col++)	/* Set get function per vector */
		api_get_val[col] = api_select_get_function (GMT->parent, V->type[col]);

	for (row = 0; row < V->n_rows; row++) {
		for (col = 0; col < V->n_columns; col++)
			api_get_val[col] (&(V->data[col]), row, &(GMT->current.io.curr_rec[col]));
		if (api_bin_input_memory (GMT, V->n_columns, V->n_columns) < 0)	/* Segment header found, finish the segment we worked on and goto next */
			gmt_write_segmentheader (GMT, fp, V->n_columns);
		else {/* Format an ascii record for output */
			fprintf (fp, GMT->current.setting.format_float_out, GMT->current.io.curr_rec[0]);
			for (col = 1; col < V->n_columns; col++) {
				fprintf (fp, "%s", GMT->current.setting.io_col_separator);
				fprintf (fp, GMT->current.setting.format_float_out, GMT->current.io.curr_rec[col]);
			}
			fprintf (fp, "\n");
		}
	}
	gmt_M_free (GMT, api_get_val);

	if (close_file) fclose (fp);
	return (GMT_NOERROR);
}

/*! . */
GMT_LOCAL int api_export_vector (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_VECTOR *V_obj) {
	/* Does the actual work of writing out the specified Matrix to a destination.  Only FILE supported for testing.
	 * The mode not used yet.
	 */
	int item, error;
	unsigned int kind;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_vector: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_VECTOR, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* This is the API object for the output destination */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET)) {	/* Only allow writing of a data set once, unless we override by resetting the mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	}
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;

	/* Passed sanity and allowed to write */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* api_write_vector will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write VECTOR to %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((error = api_write_vector (GMT, S_obj->filename, S_obj->method, mode, V_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_STREAM:
			/* api_write_vector will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_OUT]) ? 0 : 1;	/* For message only: 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write VECTOR to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = api_write_vector (GMT, S_obj->fp, S_obj->method, mode, V_obj))) return (gmtapi_report_error (API, error));
			break;
	 	case GMT_IS_FDESC:
			/* api_write_vector will report where it is writing from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_OUT) ? 0 : 1;	/* For message only: 0 if stdout, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Write VECTOR to %s %s output stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((error = api_write_vector (GMT, S_obj->fp, S_obj->method, mode, V_obj))) return (gmtapi_report_error (API, error));
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export VECTOR\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as written */
	S_obj->data = NULL;

	return GMT_OK;
}

GMT_LOCAL struct GMT_VECTOR *api_read_vector (struct GMT_CTRL *GMT, void *source, unsigned int src_type, unsigned int mode) {
	/* We read the VECTOR from fp [or stdin].
	 * src_type can be GMT_IS_[FILE|STREAM|FDESC]
	 * mode is not used yet.  We only do ascii file for now - later need to deal with -b
	 */

	bool close_file = false, first = true, add_first_segheader = false;
	uint64_t row = 0, col, dim[GMT_DIM_SIZE] = {0, 0, GMT->current.setting.export_type, 0};
	char V_file[GMT_BUFSIZ] = {""};
	char line[GMT_BUFSIZ] = {""};
	FILE *fp = NULL;
	struct GMT_VECTOR *V = NULL;
	GMT_putfunction api_put_val = NULL;
	gmt_M_unused(mode);

	if (src_type == GMT_IS_FILE && !source) src_type = GMT_IS_STREAM;	/* No filename given, default to stdin */

	if (src_type == GMT_IS_FILE) {	/* dest is a file name */
		strncpy (V_file, source, GMT_BUFSIZ-1);
		if ((fp = fopen (V_file, "r")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot open Vector file %s\n", V_file);
			return_null (GMT->parent, GMT_ERROR_ON_FOPEN);
		}
		close_file = true;	/* We only close files we have opened here */
	}
	else if (src_type == GMT_IS_STREAM) {	/* Open file pointer given, just copy */
		fp = (FILE *)source;
		if (fp == NULL) fp = GMT->session.std[GMT_IN];	/* Default destination */
		if (fp == GMT->session.std[GMT_IN])
			strcpy (V_file, "<stdin>");
		else
			strcpy (V_file, "<input stream>");
	}
	else if (src_type == GMT_IS_FDESC) {		/* Open file descriptor given, just convert to file pointer */
		int *fd = source;
		if (fd && (fp = fdopen (*fd, "r")) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot convert Vector file descriptor %d to stream in api_read_vector\n", *fd);
			return_null (GMT->parent, GMT_ERROR_ON_FDOPEN);
		}
		if (fd == NULL) fp = GMT->session.std[GMT_IN];	/* Default destination */
		if (fp == GMT->session.std[GMT_IN])
			strcpy (V_file, "<stdin>");
		else
			strcpy (V_file, "<input file descriptor>");
		close_file = true;	/* since fdopen allocates space */
	}
	else {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unrecognized source type %d in api_read_vector\n", src_type);
		return_null (GMT->parent, GMT_NOT_A_VALID_METHOD);
	}
	GMT_Report (GMT->parent, GMT_MSG_DEBUG, "Read Vector from %s\n", V_file);

	while (fgets (line, GMT_BUFSIZ, fp)) {
		if (line[0] == '#') continue;	/* Just skip headers */
		if (line[0] == '>') {
			if (first) {	/* Have not allocated yet so just skip that row for now */
				first = false;
				add_first_segheader = true;
			}
			else {	/* Already allocated so place NaNs */
				gmt_prep_tmp_arrays (GMT, row, dim[0]);	/* Init or reallocate tmp vectors */
				for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][row] = GMT->session.d_NaN;
			}
		}
		else {	/* Regular data record */
			gmt_chop (line);	/* Remove linefeeds */
			dim[0] = gmtlib_conv_text2datarec (GMT, line, GMT_BUFSIZ, GMT->current.io.curr_rec);
			gmt_prep_tmp_arrays (GMT, row, dim[0]);	/* Init or reallocate tmp vectors */
			for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][row] = GMT->current.io.curr_rec[col];
		}
		row++;
	}
	if (add_first_segheader) for (col = 0; col < dim[0]; col++) GMT->hidden.mem_coord[col][0] = GMT->session.d_NaN;
	dim[1] = row;	/* Allocate all vectors using current type setting in the defaults [GMT_DOUBLE] */
	if ((V = GMT_Create_Data (GMT->parent, GMT_IS_VECTOR, GMT_IS_POINT, 0, dim, NULL, NULL, 0, 0, NULL)) == NULL) {
		if (close_file) fclose (fp);
		return_null (GMT->parent, GMT_MEMORY_ERROR);
	}
	for (col = 0; col < V->n_columns; col++) {
		api_put_val = api_select_put_function (GMT->parent, V->type[col]);
		for (row = 0; row < V->n_rows; row++)
			api_put_val (&(V->data[col]), row, GMT->hidden.mem_coord[col][row]);
	}

	if (close_file) fclose (fp);
	return (V);
}

/*! . */
GMT_LOCAL struct GMT_VECTOR * api_import_vector (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in a GMT vector table.
	 */

	int item;
	unsigned int kind;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_vector: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return_null (API, GMT_NO_INPUT);
	if ((item = gmtapi_validate_id (API, GMT_IS_VECTOR, object_ID, GMT_IN, GMTAPI_OPTION_INPUT)) == GMT_NOTSET)
		return_null (API, API->error);

	S_obj = API->object[item];	/* Use S_obj as shorthand */
	if (S_obj->status != GMT_IS_UNUSED) { /* Already read this resource before; are we allowed to re-read? */
		if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC)
			return_null (API, GMT_READ_ONCE); /* Not allowed to re-read streams */
		if (!(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
	}

	/* Passed sanity and allowed to read */

	switch (S_obj->method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading VECTOR from %s %s\n", GMT_method[S_obj->method], S_obj->filename);
			if ((V_obj = api_read_vector (GMT, S_obj->filename, S_obj->method, mode)) == NULL)
				return_null (API, GMT_DATA_READ_ERROR);
			break;
		case GMT_IS_STREAM:
 			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (S_obj->fp == GMT->session.std[GMT_IN]) ? 0 : 1;	/* For message only: 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading VECTOR from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((V_obj = api_read_vector (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_DATA_READ_ERROR);
			break;
		case GMT_IS_FDESC:
			/* api_read_vector will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			kind = (*((int *)S_obj->fp) == GMT_IN) ? 0 : 1;	/* For message only: 0 if stdin, 1 otherwise for user pointer */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading VECTOR from %s %s stream\n", GMT_method[S_obj->method], GMT_stream[kind]);
			if ((V_obj = api_read_vector (GMT, S_obj->fp, S_obj->method, mode)) == NULL)
				return_null (API, GMT_CPT_READ_ERROR);
			break;
		case GMT_IS_DUPLICATE:	/* Duplicate the input VECTOR */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating VECTOR from VECTOR memory location [NOT IMPLEMENTED YET]\n");
			if (S_obj->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
			//V_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_VECTOR);
			//gmtlib_copy_vector (GMT, V_obj, S_obj->resource);
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location, so nothing is allocated */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing VECTOR from VECTOR memory location\n");
			if ((V_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			break;
		default:	/* Barking up the wrong tree here... */
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import VECTOR\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}
	S_obj->status = GMT_IS_USED;	/* Mark as read */
	S_obj->data = V_obj;		/* Retain pointer to the allocated data so we use garbage collection later */
	return (V_obj);	/* Pass back the vector */
}

#if 0
GMT_LOCAL bool api_col_check (struct GMT_DATATABLE *T, uint64_t *n_cols) {
	uint64_t seg;
	/* Checks that all segments in this table has the correct number of columns.
	 * If *n_cols == 0 we set it to the number of columns found in the first segment. */

	for (seg = 0; seg < T->n_segments; seg++) {
		if ((*n_cols) == 0 && seg == 0) *n_cols = T->segment[seg]->n_columns;
		if (T->segment[seg]->n_columns != (*n_cols)) return (true);
	}
	return (false);	/* All is well */
}
#endif

/*! . */
GMT_LOCAL void api_increment_d (struct GMT_DATASET *D_obj, uint64_t n_rows, uint64_t n_columns, uint64_t n_seg) {
	/* Increment dimensions for this single dataset's single table's last segment */
	uint64_t last_seg = n_seg - 1;
	assert (n_seg > 0);
	D_obj->table[D_obj->n_tables]->segment[last_seg]->n_rows = n_rows;
	D_obj->table[D_obj->n_tables]->segment[last_seg]->n_columns = D_obj->table[D_obj->n_tables]->n_columns = n_columns;
	D_obj->table[D_obj->n_tables]->n_records += n_rows;
	D_obj->table[D_obj->n_tables]->n_segments = n_seg;
	D_obj->n_tables++;	/* Since we just read one table */
}

/*! . */
GMT_LOCAL struct GMT_DATASET *api_import_dataset (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in the entire virtual data set (possibly via many sources)
	 * If object_ID == GMT_NOTSET we get all registered input tables, otherwise we just get the one requested.
	 * Note: Memory is allocated for the Dataset except for method GMT_IS_REFERENCE.
	 */

	int item, first_item = 0, this_item = GMT_NOTSET, last_item, new_item, new_ID, status;
	unsigned int geometry = GMT_IS_PLP, n_used = 0, method;
	bool allocate = false, update = false, diff_types, use_GMT_io, greenwich = true;
	bool via = false, got_data = false;
	size_t n_alloc, s_alloc = GMT_SMALL_CHUNK;
	uint64_t row, seg, col, ij, n_records = 0, n_columns = 0, col_pos, n_use;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_getfunction api_get_val = NULL;
	struct GMT_DATASET *D_obj = NULL, *Din_obj = NULL;
	struct GMT_DATASEGMENT *S = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_dataset: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) {	/* Means there is more than one source: Merge all registered data tables into a single virtual data set */
		last_item = API->n_objects - 1;	/* Must check all registered objects */
		allocate = true;
		n_alloc = GMT_TINY_CHUNK;	/* We don't expect that many files to be given initially */
	}
	else {		/* Requested a single, specific data table/file */
		int flag = (API->module_input) ? GMTAPI_MODULE_INPUT : GMTAPI_OPTION_INPUT;	/* Needed by Validate_ID */
		if ((first_item = gmtapi_validate_id (API, GMT_IS_DATASET, object_ID, GMT_IN, flag)) == GMT_NOTSET)
			return_null (API, API->error);
		last_item = first_item;
		n_alloc = 1;
	}

	/* Allocate a single data set and an initial allocated list of n_alloc tables */
	D_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_DATASET);
	D_obj->table = gmt_M_memory (GMT, NULL, n_alloc, struct GMT_DATATABLE *);
	D_obj->alloc_mode = GMT_ALLOC_INTERNALLY;	/* So GMT_* modules can free this memory (may override below) */
	D_obj->alloc_level = GMT->hidden.func_level;	/* So GMT_* modules can free this memory (may override below) */
	use_GMT_io = !(mode & GMT_IO_ASCII);		/* false if we insist on ASCII reading */
	GMT->current.io.seg_no = GMT->current.io.rec_no = GMT->current.io.rec_in_tbl_no = 0;	/* Reset for each new dataset */
	if (GMT->common.R.active[RSET] && GMT->common.R.wesn[XLO] < -180.0 && GMT->common.R.wesn[XHI] > -180.0) greenwich = false;

	for (item = first_item; item <= last_item; item++) {	/* Look through all sources for registered inputs (or just one) */
		S_obj = API->object[item];	/* S_obj is the current data object */
		if (!S_obj) {	/* Probably not a good sign. NOTE: Probably cannot happen since skipped in api_next_source, no? */
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_dataset: Skipped empty object (item = %d)\n", item);
			continue;
		}
		if (!S_obj->selected) continue;			/* Registered, but not selected */
		if (S_obj->direction == GMT_OUT) continue;	/* We're doing reading here, so skip output objects */
		if (S_obj->family != GMT_IS_DATASET) continue;	/* We're doing datasets here, so skip other data types */
		if (API->module_input && !S_obj->module_input) continue;	/* Do not mix module-inputs and option inputs if knowable */
		if (S_obj->status != GMT_IS_UNUSED) { 	/* Already read this resource before; are we allowed to re-read? */
			if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC) {
				gmt_M_free (GMT, D_obj->table);		gmt_M_free (GMT, D_obj);
				return_null (API, GMT_READ_ONCE);	/* Cannot re-read streams */
			}
			if (!(mode & GMT_IO_RESET)) {
				gmt_M_free (GMT, D_obj->table);		gmt_M_free (GMT, D_obj);
				return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
			}
		}
		if (this_item == GMT_NOTSET) this_item = item;	/* First item that worked */
		via = false;
		geometry = (GMT->common.a.output) ? GMT->common.a.geometry : S_obj->geometry;	/* When reading GMT and writing OGR/GMT we must make sure we set this first */
		method = api_set_method (S_obj);	/* Get the actual method to use */
		switch (method) {	/* File, array, stream, reference, etc ? */
	 		case GMT_IS_FILE:	/* Import all the segments, then count total number of records */
#ifdef SET_IO_MODE
				if (item == first_item) gmt_setmode (GMT, GMT_IN);	/* Windows may need to switch read mode from text to binary */
#endif
				/* gmtlib_read_table will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
				if (GMT->current.io.ogr == GMT_OGR_TRUE && D_obj->n_tables > 0) {	/* Only single tables if GMT/OGR */
					gmt_M_free (GMT, D_obj->table);		gmt_M_free (GMT, D_obj);
					return_null (API, GMT_OGR_ONE_TABLE_ONLY);
				}
				GMT_Report (API, GMT_MSG_LONG_VERBOSE,
				            "Reading %s from %s %s\n", GMT_family[S_obj->family], GMT_method[S_obj->method], S_obj->filename);
				if ((D_obj->table[D_obj->n_tables] = gmtlib_read_table (GMT, S_obj->filename, S_obj->method, greenwich, &geometry, use_GMT_io)) == NULL)
					continue;		/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				D_obj->table[D_obj->n_tables]->id = D_obj->n_tables;	/* Give sequential internal object_ID numbers to tables */
				D_obj->n_tables++;	/* Since we just read one */
				update = true;		/* Have reason to update min/max when done */
				break;

			case GMT_IS_STREAM:	/* Import all the segments, then count total number of records */
	 		case GMT_IS_FDESC:
				/* gmtlib_read_table will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
#ifdef SET_IO_MODE
				if (item == first_item) gmt_setmode (GMT, GMT_IN);	/* Windows may need to switch read mode from text to binary */
#endif
				if (GMT->current.io.ogr == GMT_OGR_TRUE && D_obj->n_tables > 0)	{	/* Only single tables if GMT/OGR */
					gmt_M_free (GMT, D_obj);	return_null (API, GMT_OGR_ONE_TABLE_ONLY);
				}
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading %s from %s %" PRIxS "\n", GMT_family[S_obj->family], GMT_method[S_obj->method], (size_t)S_obj->fp);
				if ((D_obj->table[D_obj->n_tables] = gmtlib_read_table (GMT, S_obj->fp, S_obj->method, greenwich, &geometry, use_GMT_io)) == NULL) continue;		/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				D_obj->table[D_obj->n_tables]->id = D_obj->n_tables;	/* Give sequential internal object_ID numbers to tables */
				D_obj->n_tables++;	/* Since we just read one */
				update = true;		/* Have reason to update min/max when done */
				break;

			case GMT_IS_DUPLICATE:	/* Duplicate the input dataset */
				gmt_M_free (GMT, D_obj->table);	/* Free up what we allocated earlier since gmt_duplicate_dataset does it all */
				gmt_M_free (GMT, D_obj);
				if (n_used) return_null (API, GMT_ONLY_ONE_ALLOWED);
				if ((Din_obj = S_obj->resource) == NULL)	/* Ooops, noting there? */
					return_null (API, GMT_PTR_IS_NULL);
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table from GMT_DATASET memory location\n");
				D_obj = gmt_duplicate_dataset (GMT, Din_obj, GMT_ALLOC_NORMAL, NULL);
				break;

			case GMT_IS_REFERENCE:	/* Just pass memory location, so free up what we allocated earlier */
				gmt_M_free (GMT, D_obj->table);	/* Free up what we allocated up front since we just wish to pass the pointer */
				gmt_M_free (GMT, D_obj);
				if (n_used) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing data table from GMT_DATASET memory location\n");
				if ((D_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				break;

		 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:
		 	case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
				/* Each matrix source becomes a separate table with a single segment unless there are NaN-records as segment headers */
				if ((M_obj = S_obj->resource) == NULL) {
					gmt_M_free (GMT, D_obj);
					return_null (API, GMT_PTR_IS_NULL);
				}
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table from user matrix location\n");
				/* Allocate a table with a single segment given matrix dimensions, but if nan-record we may end up with more segments */
				n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : M_obj->n_columns;
				D_obj->table[D_obj->n_tables] = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
				D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, NULL, s_alloc, struct GMT_DATASEGMENT *);
				S = D_obj->table[D_obj->n_tables]->segment[0] =
				    GMT_Alloc_Segment (API, GMT_IS_DATASET, M_obj->n_rows, n_columns, NULL, NULL);
				GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
				api_get_val = api_select_get_function (API, M_obj->type);
				n_use = api_n_cols_needed_for_gaps (GMT, M_obj->n_columns);	/* Number of input columns to process */
				for (row = seg = n_records = 0; row < M_obj->n_rows; row++) {	/* This loop may include NaN-records and data records */
					api_update_prev_rec (GMT, n_use);	/* Make last current record the previous record if it is required by gap checking */
					for (col = 0; col < M_obj->n_columns; col++) {	/* Extract cols for a single record and store result in curr_rec */
						ij = GMT_2D_to_index (row, col, M_obj->dim);	/* Index into the user data matrix depends on layout (M->shape) */
						api_get_val (&(M_obj->data), ij, &(GMT->current.io.curr_rec[col]));
					}
					/* Now process the current record */
					if ((status = api_bin_input_memory (GMT, M_obj->n_columns, n_use)) < 0) {	/* Segment header found, finish the segment we worked on and goto next */
						if (status == GMTAPI_GOT_SEGGAP) API->current_rec[GMT_IN]--;	/* Since we inserted a segment header we must revisit this record as the first in next segment */
						if (got_data) {	/* If first input segment has header then we already have that segment allocated */
							(void)GMT_Alloc_Segment (API, GMT_IS_DATASET, n_records, n_columns, NULL, S);	/* Reallocate to exact length */
							D_obj->table[D_obj->n_tables]->n_records += n_records;			/* Update record count for this table */
							seg++;	/* Increment number of segments */
							if (seg == s_alloc) {	/* Allocate more space for additional segments */
								s_alloc <<= 1;	/* Double current alloc limit for segments, then allocate space for more segments */
								D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, D_obj->table[D_obj->n_tables]->segment, s_alloc, struct GMT_DATASEGMENT *);
							}
							/* Allocate next segment with initial size the remainder of the data, which is the maximum length possible */
							S = D_obj->table[D_obj->n_tables]->segment[seg] = GMT_Alloc_Segment (API, GMT_IS_DATASET, M_obj->n_rows-n_records, n_columns, NULL, NULL);
							n_records = 0;	/* This is number of recs in current segment so we reset it to zero */
						}
					}
					else {	/* Found a data record */
						for (col = 0; col < n_columns; col++)	/* Place the record into the dataset segment structure */
							S->data[col][n_records] = api_get_record_value (GMT, GMT->current.io.curr_rec, col, M_obj->n_columns);
						got_data = true;	/* No longer before first data record */
						n_records++;	/* Update count of records in current segment */
					}
				}
				if (seg)	/* Got more than one segment, so finalize the reallocation of last segment to exact record count */
					(void)GMT_Alloc_Segment (API, GMT_IS_DATASET, n_records, n_columns, NULL, S);	/* Reallocate to exact length */
				seg++;	/* Now holds the total number of segments */
				/* Realloc this table's segment array to the actual length [i.e., seg] */
				D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, D_obj->table[D_obj->n_tables]->segment, seg, struct GMT_DATASEGMENT *);
				api_increment_d (D_obj, n_records, n_columns, seg);	/* Update counters for D_obj's only table */
				new_ID = GMT_Register_IO (API, GMT_IS_DATASET, GMT_IS_DUPLICATE, geometry, GMT_IN, NULL, D_obj);	/* Register a new resource to hold D_obj */
				if ((new_item = gmtapi_validate_id (API, GMT_IS_DATASET, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
					return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
				API->object[new_item]->data = D_obj;
				API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
				D_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
				D_obj->geometry = S_obj->geometry;	/* Since provided when registered */
				update = via = true;
				break;

	 		case GMT_IS_DUPLICATE|GMT_VIA_VECTOR:
				/* Each column array source becomes column arrays in a separate table with one (or more if NaN-records) segments */
				if ((V_obj = S_obj->resource) == NULL) {
					gmt_M_free (GMT, D_obj);	return_null (API, GMT_PTR_IS_NULL);
				}
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table from user %" PRIu64 " column arrays of length %" PRIu64 "\n",
				            V_obj->n_columns, V_obj->n_rows);
				/* Allocate a single table with one segment - there may be more if there are nan-records */
				n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : V_obj->n_columns;
				D_obj->table[D_obj->n_tables] = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
				D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, NULL, s_alloc, struct GMT_DATASEGMENT *);
				S = D_obj->table[D_obj->n_tables]->segment[0] = GMT_Alloc_Segment (API, GMT_IS_DATASET, V_obj->n_rows, n_columns, NULL, NULL);
				for (col = 1, diff_types = false; !diff_types && col < V_obj->n_columns; col++) if (V_obj->type[col] != V_obj->type[col-1]) diff_types = true;
				api_get_val = api_select_get_function (API, V_obj->type[0]);
				for (row = seg = n_records = 0; row < V_obj->n_rows; row++) {	/* This loop may include NaN-records and data records */
					n_use = api_n_cols_needed_for_gaps (GMT, V_obj->n_columns);
					api_update_prev_rec (GMT, n_use);
					for (col = 0; col < V_obj->n_columns; col++) {	/* Process a single record into curr_rec */
						if (diff_types) api_get_val = api_select_get_function (API, V_obj->type[col]);
						api_get_val (&(V_obj->data[col]), row, &(GMT->current.io.curr_rec[col]));
					}
					if ((status = api_bin_input_memory (GMT, V_obj->n_columns, n_use)) < 0) {	/* Segment header found, finish the one we had and add more */
						if (status == GMTAPI_GOT_SEGGAP) API->current_rec[GMT_IN]--;	/* Since we inserted a segment header we must revisit this record as first in next segment */
						if (got_data) {	/* If first input segment has header then we already have a segment allocated */
							(void)GMT_Alloc_Segment (API, GMT_IS_DATASET, n_records, n_columns, NULL, S);
							D_obj->table[D_obj->n_tables]->n_records += n_records;
							seg++;	/* Increment number of segments */
							if (seg == s_alloc) {	/* Allocate more space for segments */
								s_alloc <<= 1;
								D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, D_obj->table[D_obj->n_tables]->segment, s_alloc, struct GMT_DATASEGMENT *);
							}
							/* Allocate next segment with initial size the remainder of the data */
							S = D_obj->table[D_obj->n_tables]->segment[seg] = GMT_Alloc_Segment (API, GMT_IS_DATASET, V_obj->n_rows-n_records, n_columns, NULL, NULL);
							n_records = 0;	/* This is number of recs in current segment */
						}
					}
					else {	/* Data record */
						for (col = 0; col < n_columns; col++)	/* Place the record into the structure */
							S->data[col][n_records] = api_get_record_value (GMT, GMT->current.io.curr_rec, col, V_obj->n_columns);
						got_data = true;
						n_records++;
					}
				}
				if (seg)	/* Got more than one segment, finalize the realloc of last segment */
					(void)GMT_Alloc_Segment (API, GMT_IS_DATASET, n_records, n_columns, NULL, S);	/* Reallocate to exact length */
				seg++;	/* Total number of segments */
				/* Realloc this table's segment array to the actual length [i.e., seg] */
				D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, D_obj->table[D_obj->n_tables]->segment, seg, struct GMT_DATASEGMENT *);
				api_increment_d (D_obj, n_records, n_columns, seg);	/* Update counters for D_obj's only table */
				new_ID = GMT_Register_IO (API, GMT_IS_DATASET, GMT_IS_DUPLICATE, geometry, GMT_IN, NULL, D_obj);	/* Register a new resource to hold D_obj */
				if ((new_item = gmtapi_validate_id (API, GMT_IS_DATASET, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
					return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
				API->object[new_item]->data = D_obj;
				API->object[new_item]->status = GMT_IS_USED;			/* Mark as read */
				D_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
				D_obj->geometry = S_obj->geometry;	/* Since provided when registered */
				update = via = true;
				break;

		 	case GMT_IS_REFERENCE|GMT_VIA_VECTOR:
				if ((V_obj = S_obj->resource) == NULL) {
					gmt_M_free (GMT, D_obj);	return_null (API, GMT_PTR_IS_NULL);
				}
				if (V_obj->type[0] != GMT_DOUBLE) {
					gmt_M_free (GMT, D_obj);	return_null (API, GMT_NOT_A_VALID_TYPE);
				}
				/* Each column double array source becomes preallocated column arrays in a separate table with a single segment */
				n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : V_obj->n_columns;
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing data table from user %" PRIu64 " column arrays of length %" PRIu64 "\n",
				            V_obj->n_columns, V_obj->n_rows);
				D_obj->table[D_obj->n_tables] = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
				D_obj->table[D_obj->n_tables]->segment = gmt_M_memory (GMT, NULL, 1, struct GMT_DATASEGMENT *);
				D_obj->table[D_obj->n_tables]->segment[0] = GMT_Alloc_Segment (API, GMT_IS_DATASET, 0, n_columns, NULL, NULL);
				for (col = 0; col < V_obj->n_columns; col++) {
					if (GMT->common.i.active)	/* -i has selected some columns */
						col_pos = GMT->current.io.col[GMT_IN][col].col;	/* Which data column to pick */
					else if (GMT->current.setting.io_lonlat_toggle[GMT_IN] && col < GMT_Z)	/* Worry about -: for lon,lat */
						col_pos = 1 - col;	/* Read lat/lon instead of lon/lat */
					else
						col_pos = col;	/* Just goto that column */
					D_obj->table[D_obj->n_tables]->segment[0]->data[col] = V_obj->data[col_pos].f8;
				}
				api_increment_d (D_obj, V_obj->n_rows, n_columns, 1U);	/* Update counters for D_obj with 1 segment */
				D_obj->alloc_mode = GMT_ALLOC_EXTERNALLY;	/* Since we just hooked on the arrays */
				new_ID = GMT_Register_IO (API, GMT_IS_DATASET, GMT_IS_REFERENCE, geometry, GMT_IN, NULL, D_obj);	/* Register a new resource to hold D_obj */
				if ((new_item = gmtapi_validate_id (API, GMT_IS_DATASET, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
					return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
				API->object[new_item]->data = D_obj;
				API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
				D_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
				D_obj->geometry = S_obj->geometry;	/* Since provided when registered */
				S_obj->family = GMT_IS_VECTOR;	/* Done with the via business now */
				update = via = true;
				break;

			default:	/* Barking up the wrong tree here... */
				GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import data tables\n");
				gmt_M_free (GMT, D_obj->table);
				gmt_M_free (GMT, D_obj);
				return_null (API, GMT_NOT_A_VALID_METHOD);
				break;
		}
		if (update) {	/* Means we got stuff and need to update the total dataset statistics so far */
			D_obj->n_segments += D_obj->table[D_obj->n_tables-1]->n_segments;	/* Sum up total number of segments in the entire data set */
			D_obj->n_records  += D_obj->table[D_obj->n_tables-1]->n_records;		/* Sum up total number of records in the entire data set */
			/* Update segment IDs so they are sequential across many tables (gmtlib_read_table sets the ids relative to current table). */
			if (D_obj->n_tables > 1) {
				for (seg = 0; seg < D_obj->table[D_obj->n_tables-1]->n_segments; seg++)
					D_obj->table[D_obj->n_tables-1]->segment[seg]->id += D_obj->table[D_obj->n_tables-2]->n_segments;
			}
			if (allocate && D_obj->n_tables == n_alloc) {	/* Must allocate more space for additional tables */
				size_t old_n_alloc = n_alloc;
				n_alloc += GMT_TINY_CHUNK;
				D_obj->table = gmt_M_memory (GMT, D_obj->table, n_alloc, struct GMT_DATATABLE *);
				gmt_M_memset (&(D_obj->table[old_n_alloc]), n_alloc - old_n_alloc, struct GMT_DATATABLE *);	/* Set new memory to NULL */
			}
#ifdef GMT_BACKWARDS_API
			for (seg = 0; seg < D_obj->table[D_obj->n_tables-1]->n_segments; seg++)
				D_obj->table[D_obj->n_tables-1]->segment[seg]->coord = D_obj->table[D_obj->n_tables-1]->segment[seg]->data;
#endif
		}
		S_obj->alloc_mode = D_obj->alloc_mode;	/* Clarify allocation mode for this object */
#if 0
		if (api_col_check (D_obj->table[D_obj->n_tables-1], &n_cols)) {	/* Different tables have different number of columns, which is not good */
			return_null (API, GMT_N_COLS_VARY);
		}
#endif
		S_obj->status = GMT_IS_USED;	/* Mark input object as read */
		n_used++;	/* Number of items actually processed */
	}
	if (D_obj->n_tables == 0) {	/* Only found empty files (e.g., /dev/null) and we have nothing to show for our efforts.  Return an single empty table with no segments. */
		D_obj->table = gmt_M_memory (GMT, D_obj->table, 1, struct GMT_DATATABLE *);
		D_obj->table[0] = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
		D_obj->n_tables = 1;	/* But we must indicate we found one (empty) table */
	}
	else {	/* Found one or more tables, finalize table allocation, set number of columns, and possibly allocate min/max arrays if not there already */
		if (allocate && D_obj->n_tables < n_alloc) D_obj->table = gmt_M_memory (GMT, D_obj->table, D_obj->n_tables, struct GMT_DATATABLE *);
		D_obj->n_columns = D_obj->table[0]->n_columns;
		if (!D_obj->min) D_obj->min = gmt_M_memory (GMT, NULL, D_obj->n_columns, double);
		if (!D_obj->max) D_obj->max = gmt_M_memory (GMT, NULL, D_obj->n_columns, double);
	}
	D_obj->geometry = geometry;			/* Since gmtlib_read_table may have changed it */
	gmtlib_set_dataset_minmax (GMT, D_obj);	/* Set the min/max values for the entire dataset */
	if (!via) API->object[this_item]->data = D_obj;	/* Retain pointer to the allocated data so we use garbage collection later */
	return (D_obj);
}

/*! . */
GMT_LOCAL int api_destroy_data_ptr (struct GMTAPI_CTRL *API, enum GMT_enum_family family, void *ptr) {
	/* Like GMT_Destroy_Data but takes pointer to data rather than address of pointer.
	 * We pass true to make sure we free the memory.  Some objects (grid, matrix, vector) may
	 * point to externally allocated memory so we return the alloc_mode for those items.
	 * This is mostly for information since the pointers to such external memory have now
	 * been set to NULL instead of being freed.
	 * The containers are always allocated by GMT so those are freed at the end.
	 */

	struct GMT_CTRL *GMT;
	if (API == NULL) return (GMT_NOT_A_SESSION);
	if (!ptr) return (GMT_OK);	/* Null pointer */
	GMT = API->GMT;

	switch (family) {
		case GMT_IS_GRID:
			gmtgrdio_free_grid_ptr (GMT, ptr, true);
			break;
		case GMT_IS_DATASET:
			gmtlib_free_dataset_ptr (GMT, ptr);
			break;
		case GMT_IS_TEXTSET:
			gmtlib_free_textset_ptr (GMT, ptr);
			break;
		case GMT_IS_PALETTE:
			gmtlib_free_cpt_ptr (GMT, ptr);
			break;
		case GMT_IS_IMAGE:
			gmtlib_free_image_ptr (GMT, ptr, true);
			break;
		case GMT_IS_POSTSCRIPT:
			gmtlib_free_ps_ptr (GMT, ptr);
			break;
		case GMT_IS_COORD:
			/* Nothing to do as gmt_M_free below will do it */
			break;

		/* Also allow destroying of intermediate vector and matrix containers */
		case GMT_IS_MATRIX:
			gmtlib_free_matrix_ptr (GMT, ptr, true);
			break;
		case GMT_IS_VECTOR:
			gmtlib_free_vector_ptr (GMT, ptr, true);
			break;
		default:
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_FAMILY));
			break;
	}
	gmt_M_free (GMT, ptr);	/* OK to free container */
	return (GMT_OK);	/* Null pointer */
}

/*! . */
GMT_LOCAL int api_export_dataset (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_DATASET *D_obj) {
 	/* Does the actual work of writing out the specified data set to a single destination.
	 * If object_ID == GMT_NOTSET we use the first registered output destination, otherwise we just use the one specified.
	 * See the GMT API documentation for how mode is used to create multiple files from segments or tables of a dataset.
	 */
	int item, error, default_method;
	unsigned int method;
	uint64_t tbl, col, row_out, row, seg, ij;
	bool save;
	double value;
	p_func_uint64_t GMT_2D_to_index = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_DATASET *D_copy = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMT_DATASEGMENT *S = NULL;
	struct GMT_CTRL *GMT = API->GMT;
	void *ptr = NULL;
	GMT_putfunction api_put_val = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_dataset: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_DATASET, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* S is the object whose data we will export */
	if (S_obj->family != GMT_IS_DATASET) return (gmtapi_report_error (API, GMT_NOT_A_VALID_FAMILY));	/* Called with wrong data type */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))	/* Only allow writing of a data set once unless overridden by mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;	/* Remove the reset bit */
	if (mode >= GMT_WRITE_TABLE && !S_obj->filename) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));	/* Must have filename when segments are to be written */
	default_method = GMT_IS_FILE;
	if (S_obj->filename)	/* Write to this file */
		ptr = S_obj->filename;
	else {			/* No filename so we switch default method to writing to a stream or fdesc */
		default_method = (S_obj->method == GMT_IS_FILE) ? GMT_IS_STREAM : S_obj->method;
		ptr = S_obj->fp;
#ifdef SET_IO_MODE
		gmt_setmode (GMT, GMT_OUT);	/* Windows may need to switch write mode from text to binary */
#endif
	}
	gmtlib_set_dataset_minmax (GMT, D_obj);	/* Update all counters and min/max arrays */
	D_obj->io_mode = mode;	/* Handles if tables or segments should be written to separate files, according to mode */
	method = api_set_method (S_obj);	/* Get the actual method to use */
	switch (method) {	/* File, array, stream, etc. */
	 	case GMT_IS_STREAM:
#ifdef SET_IO_MODE
			gmt_setmode (GMT, GMT_OUT);	/* Windows may need to switch write mode from text to binary */
#endif
		case GMT_IS_FILE:
	 	case GMT_IS_FDESC:
			/* gmtlib_write_dataset (or lower) will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			if ((error = gmtlib_write_dataset (GMT, ptr, default_method, D_obj, true, GMT_NOTSET))) return (gmtapi_report_error (API, error));
			break;

		case GMT_IS_DUPLICATE:		/* Duplicate the input dataset on output */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table to GMT_DATASET memory location\n");
			D_copy = gmt_duplicate_dataset (GMT, D_obj, GMT_ALLOC_NORMAL, NULL);
			S_obj->resource = D_copy;	/* Set resource pointer from object to this dataset */
			break;

		case GMT_IS_REFERENCE:	/* Just pass memory location */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing data table to GMT_DATASET memory location\n");
			gmtlib_change_dataset (GMT, D_obj);	/* Deal with any -o settings */
			S_obj->resource = D_obj;			/* Set resource pointer from object to this dataset */
			D_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			break;

		case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:
		case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table to user matrix location\n");
			M_obj = gmtlib_create_matrix (GMT, 1U, GMT_OUT);	/* 1-layer matrix (i.e., 2-D) */
			/* Allocate final output space since we now know all dimensions */
			save = GMT->current.io.multi_segments[GMT_OUT];
			if (GMT->current.io.skip_headers_on_outout) GMT->current.io.multi_segments[GMT_OUT] = false;
			M_obj->n_rows = (GMT->current.io.multi_segments[GMT_OUT]) ? D_obj->n_records + D_obj->n_segments : D_obj->n_records;	/* Number of rows needed to hold the data [incl any segment headers] */
			M_obj->n_columns = (GMT->common.o.active) ? GMT->common.o.n_cols : D_obj->n_columns;					/* Number of columns needed to hold the data records */
			M_obj->dim = (M_obj->shape == GMT_IS_ROW_FORMAT) ? D_obj->n_columns : M_obj->n_rows;						/* Matrix layout order */
			S_obj->n_alloc = M_obj->n_rows * D_obj->n_columns;	/* Get total number of elements as n_rows * n_columns */
			M_obj->type = GMT->current.setting.export_type;	/* Use selected data type for the export */
			/* Allocate output matrix space or die */
			if ((error = gmtlib_alloc_univector (GMT, &(M_obj->data), M_obj->type, S_obj->n_alloc)) != GMT_OK) return (gmtapi_report_error (API, error));
			M_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
			api_put_val = api_select_put_function (API, M_obj->type);

			for (tbl = row_out = 0; tbl < D_obj->n_tables; tbl++) {	/* Loop over tables and segments */
				for (seg = 0; seg < D_obj->table[tbl]->n_segments; seg++) {
					S = D_obj->table[tbl]->segment[seg];	/* Shorthand for the current segment */
					if (GMT->current.io.multi_segments[GMT_OUT]) {	/* Must write a NaN-segment record to indicate segment break */
						for (col = 0; col < M_obj->n_columns; col++) {
							ij = GMT_2D_to_index (row_out, col, M_obj->dim);
							api_put_val (&(M_obj->data), ij, GMT->session.d_NaN);
						}
						row_out++;	/* Due to the extra NaN-data header record we just wrote */
					}
					for (row = 0; row < S->n_rows; row++, row_out++) {	/* Write this segment's data records to the matrix */
						for (col = 0; col < M_obj->n_columns; col++) {
							ij = GMT_2D_to_index (row_out, col, M_obj->dim);
							value = api_select_dataset_value (GMT, S, (unsigned int)row, (unsigned int)col);
							api_put_val (&(M_obj->data), ij, value);
						}
					}
				}
			}
			assert (M_obj->n_rows == row_out);	/* Sanity check */
			M_obj->alloc_level = S_obj->alloc_level;
			S_obj->resource = M_obj;		/* Set resource pointer from object to this matrix */
			GMT->current.io.multi_segments[GMT_OUT] = save;
			break;

		case GMT_IS_DUPLICATE|GMT_VIA_VECTOR:
		case GMT_IS_REFERENCE|GMT_VIA_VECTOR:
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table to user column arrays location\n");
			col = (GMT->common.o.active) ? GMT->common.o.n_cols : D_obj->n_columns;	/* Number of columns needed to hold the data records */
			if ((V_obj = gmt_create_vector (GMT, col, GMT_OUT)) == NULL)
				return (gmtapi_report_error (API, GMT_PTR_IS_NULL));
			save = GMT->current.io.multi_segments[GMT_OUT];
			if (GMT->current.io.skip_headers_on_outout) GMT->current.io.multi_segments[GMT_OUT] = false;
			V_obj->n_rows = (GMT->current.io.multi_segments[GMT_OUT]) ? D_obj->n_records + D_obj->n_segments : D_obj->n_records;	/* Number of data records [and any segment headers] */
			for (col = 0; col < V_obj->n_columns; col++) V_obj->type[col] = GMT->current.setting.export_type;	/* Set same data type for all columns */
			if ((error = api_alloc_vectors (GMT, V_obj, V_obj->n_rows)) != GMT_OK) return (gmtapi_report_error (API, error));	/* Allocate space for all columns */
			api_put_val = api_select_put_function (API, GMT->current.setting.export_type);	/* Since all columns are of same type we get the pointer here */
			for (tbl = row_out = 0; tbl < D_obj->n_tables; tbl++) {	/* Loop over all tables and segments */
				for (seg = 0; seg < D_obj->table[tbl]->n_segments; seg++) {
					S = D_obj->table[tbl]->segment[seg];	/* Shorthand for this segment */
					if (GMT->current.io.multi_segments[GMT_OUT]) {		/* Must write a NaN-segment record */
						for (col = 0; col < V_obj->n_columns; col++)
							api_put_val (&(V_obj->data[col]), row_out, GMT->session.d_NaN);
						row_out++;	/* Due to the extra NaN-data header */
					}
					for (row = 0; row < S->n_rows; row++, row_out++) {	/* Copy the data records */
						for (col = 0; col < V_obj->n_columns; col++) {
							value = api_select_dataset_value (GMT, S, (unsigned int)row, (unsigned int)col);
							api_put_val (&(V_obj->data[col]), row_out, value);
						}
					}
				}
			}
			assert (V_obj->n_rows == row_out);	/* Sanity check */
			V_obj->alloc_level = S_obj->alloc_level;
			S_obj->resource = V_obj;
			GMT->current.io.multi_segments[GMT_OUT] = save;
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export data tables\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->alloc_mode = D_obj->alloc_mode;	/* Clarify allocation mode for this entity */
	S_obj->status = GMT_IS_USED;	/* Mark as written */
	S_obj->data = NULL;

	return GMT_OK;
}

/*! . */
GMT_LOCAL struct GMT_TEXTSET *api_import_textset (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode) {
	/* Does the actual work of loading in the entire virtual text set (possibly via many sources)
	 * If object_ID == GMT_NOTSET we get all registered input tables, otherwise we just get the one requested.
	 * Note: Memory is allocated for the textset except for GMT_IS_REFERENCE.
	 */

	int item, first_item = 0, last_item, this_item = GMT_NOTSET, new_item, new_ID;
	unsigned int n_used = 0;
	bool update = false, allocate = false, via = false;
	size_t n_alloc;
	uint64_t row, seg;
	char *t_ptr = NULL;
	struct GMT_TEXTSET *T_obj = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_textset: Passed ID = %d and mode = %d\n", object_ID, mode);

	T_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSET);

	if (object_ID == GMT_NOTSET) {	/* More than one source: Merge all registered data tables into a single virtual text set */
		last_item = API->n_objects - 1;	/* Must check all objects */
		allocate = true;
		n_alloc = GMT_TINY_CHUNK;
	}
	else {		/* Requested a single, specific data table */
		int flag = (API->module_input) ? GMTAPI_MODULE_INPUT : GMTAPI_OPTION_INPUT;	/* Needed by Validate_ID */
		if ((first_item = gmtapi_validate_id (API, GMT_IS_TEXTSET, object_ID, GMT_IN, flag)) == GMT_NOTSET) {
			gmt_M_free (GMT, T_obj);
			return_null (API, API->error);
		}
		last_item = first_item;
		n_alloc = 1;
	}
	T_obj->table = gmt_M_memory (GMT, NULL, n_alloc, struct GMT_TEXTTABLE *);
	T_obj->alloc_mode = GMT_ALLOC_INTERNALLY;	/* So GMT_* modules can free this memory (may override below) */
	T_obj->alloc_level = GMT->hidden.func_level;	/* So GMT_* modules can free this memory (may override below) */

	for (item = first_item; item <= last_item; item++) {	/* Look through all sources for registered inputs (or just one) */
		S_obj = API->object[item];	/* Current object */
		if (!S_obj) {	/* Probably not a good sign */
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_textset: Skipped empty object (item = %d)\n", item);
			continue;
		}
		if (!S_obj->selected) continue;			/* Registered, but not selected */
		if (S_obj->direction == GMT_OUT) continue;	/* We're doing reading here, so bugger off! */
		if (S_obj->family != GMT_IS_TEXTSET) continue;	/* We're doing textsets here, so skip other things */
		if (API->module_input && !S_obj->module_input) continue;	/* Do not mix module-inputs and option inputs if knowable */
		if (S_obj->status != GMT_IS_UNUSED) {	/* Already read this resource before; are we allowed to re-read? */
			if (S_obj->method == GMT_IS_STREAM || S_obj->method == GMT_IS_FDESC) {
				gmt_M_free (GMT, T_obj->table);	gmt_M_free (GMT, T_obj);
				return_null (API, GMT_READ_ONCE);	/* Not allowed to re-read streams */
			}
			if (!(mode & GMT_IO_RESET)) {
				gmt_M_free (GMT, T_obj->table);	gmt_M_free (GMT, T_obj);
				return_null (API, GMT_READ_ONCE);	/* Not authorized to re-read */
			}
		}
		if (this_item == GMT_NOTSET) this_item = item;	/* First item that worked */
		via = false;
		switch (S_obj->method) {	/* File, array, stream, etc. */
			case GMT_IS_FILE:	/* Import all the segments, then count total number of records */
				/* gmtlib_read_texttable will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading %s from %s %s\n", GMT_family[S_obj->family], GMT_method[S_obj->method], S_obj->filename);
				if ((T_obj->table[T_obj->n_tables] = gmtlib_read_texttable (GMT, S_obj->filename, S_obj->method)) == NULL) continue;	/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				T_obj->table[T_obj->n_tables]->id = T_obj->n_tables;	/* Give internal object_ID numbers to tables */
				update = true;
				break;
	 		case GMT_IS_STREAM:
	 		case GMT_IS_FDESC:
				/* gmtlib_read_texttable will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading %s from %s %" PRIxS "\n", GMT_family[S_obj->family], GMT_method[S_obj->method], (size_t)S_obj->fp);
				if ((T_obj->table[T_obj->n_tables] = gmtlib_read_texttable (GMT, S_obj->fp, S_obj->method)) == NULL) continue;	/* Ran into an empty file (e.g., /dev/null or equivalent). Skip to next item, */
				T_obj->table[T_obj->n_tables]->id = T_obj->n_tables;	/* Give internal object_ID numbers to tables */
				update = true;
				break;
			case GMT_IS_DUPLICATE:	/* Duplicate the input dataset */
				gmt_M_free (GMT, T_obj->table);	/* Free up what we allocated since gmtlib_duplicate_textset does it all */
				gmt_M_free (GMT, T_obj);
				if (n_used) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating text table from GMT_TEXTSET memory location\n");
				if (S_obj->resource == NULL) return_null (API, GMT_PTR_IS_NULL);
				T_obj = gmtlib_duplicate_textset (GMT, S_obj->resource, GMT_ALLOC_NORMAL);
				break;
			case GMT_IS_REFERENCE:	/* Just pass memory location, so free up what we allocated first */
				gmt_M_free (GMT, T_obj->table);	/* Free up what we allocated since we will just pass the pointer */
				gmt_M_free (GMT, T_obj);
				if (n_used) return_null (API, GMT_ONLY_ONE_ALLOWED);
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing data table from GMT_TEXTSET memory location\n");
				if ((T_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
				break;
		 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:
		 	case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
				/* Each matrix source becomes a separate table with one segment */
			 	if ((M_obj = S_obj->resource) == NULL) {	/* Disaster, why is it NULL? */
					gmt_M_free (GMT, T_obj->table);	gmt_M_free (GMT, T_obj);
			 		return_null (API, GMT_PTR_IS_NULL);
			 	}
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating text table from a user matrix location\n");
				T_obj->table[T_obj->n_tables] = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTTABLE);
				T_obj->table[T_obj->n_tables]->segment = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSEGMENT *);
				T_obj->table[T_obj->n_tables]->segment[0] = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSEGMENT);
				T_obj->table[T_obj->n_tables]->segment[0]->data = gmt_M_memory (GMT, NULL, M_obj->n_rows, char *);
				t_ptr = (char *)M_obj->data.sc1;
				for (row = 0; row < M_obj->n_rows; row++)
					T_obj->table[T_obj->n_tables]->segment[0]->data[row] = strdup (&t_ptr[row*M_obj->dim]);
				T_obj->table[T_obj->n_tables]->segment[0]->n_rows = M_obj->n_rows;
				T_obj->table[T_obj->n_tables]->n_records += M_obj->n_rows;
				T_obj->table[T_obj->n_tables]->n_segments = 1;
				new_ID = GMT_Register_IO (API, GMT_IS_TEXTSET, GMT_IS_DUPLICATE, S_obj->geometry, GMT_IN, NULL, T_obj);	/* Register a new resource to hold T_obj */
				if ((new_item = gmtapi_validate_id (API, GMT_IS_DATASET, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
					return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
				API->object[new_item]->data = T_obj;
				API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
				T_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
				update = via = true;
				break;
			default:	/* Barking up the wrong tree here... */
				GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import data tables\n");
				gmt_M_free (GMT, T_obj->table);
				gmt_M_free (GMT, T_obj);
				return_null (API, GMT_NOT_A_VALID_METHOD);
				break;
		}
		if (update) {
			T_obj->n_segments += T_obj->table[T_obj->n_tables]->n_segments;	/* Sum up total number of segments across the data set */
			T_obj->n_records += T_obj->table[T_obj->n_tables]->n_records;	/* Sum up total number of records across the data set */
			/* Update segment object_IDs so they are sequential across many tables (gmtlib_read_table sets the ids relative to current table). */
			if (T_obj->n_tables > 0)
				for (seg = 0; seg < T_obj->table[T_obj->n_tables]->n_segments; seg++) {
					T_obj->table[T_obj->n_tables]->segment[seg]->id += T_obj->table[T_obj->n_tables-1]->n_segments;
#ifdef GMT_BACKWARDS_API
					T_obj->table[T_obj->n_tables]->segment[seg]->record = T_obj->table[T_obj->n_tables]->segment[seg]->data;
#endif
				}
			T_obj->n_tables++;
		}
		if (allocate && T_obj->n_tables == n_alloc) {	/* Must allocate space for more tables */
			size_t old_n_alloc = n_alloc;
			n_alloc += GMT_TINY_CHUNK;
			T_obj->table = gmt_M_memory (GMT, T_obj->table, n_alloc, struct GMT_TEXTTABLE *);
			gmt_M_memset (&(T_obj->table[old_n_alloc]), n_alloc - old_n_alloc, struct GMT_TEXTTABLE *);	/* Set to NULL */
		}
		S_obj->alloc_mode = T_obj->alloc_mode;	/* Clarify allocation mode for this entity */
		S_obj->status = GMT_IS_USED;	/* Mark as read */
		n_used++;	/* Number of items actually processed */
	}

	if (T_obj->n_tables == 0) {	/* Only found empty files (e.g., /dev/null) and we have nothing to show for our efforts.  Return an single empty table with no segments. */
		T_obj->table = gmt_M_memory (GMT, T_obj->table, 1, struct GMT_TEXTTABLE *);
		T_obj->table[0] = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTTABLE);
		T_obj->n_tables = 1;	/* But we must indicate we found one (empty) table */
	}
	else {	/* Found one or more tables */
		if (allocate && T_obj->n_tables < n_alloc) T_obj->table = gmt_M_memory (GMT, T_obj->table, T_obj->n_tables, struct GMT_TEXTTABLE *);
	}
	if (!via) T_obj->geometry = API->object[this_item]->geometry;
	API->object[this_item]->data = T_obj;		/* Retain pointer to the allocated data so we use garbage collection later */

	return (T_obj);
}

/*! . */
GMT_LOCAL int api_export_textset (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_TEXTSET *T_obj) {
	/* Does the actual work of writing out the specified text set to a single destination.
	 * If object_ID == GMT_NOTSET we use the first registered output destination, otherwise we just use the one requested.
	 * Again, see GMTAPI documentation for the meaning of mode.
	 */
	int item, error, default_method;
	uint64_t tbl, row, seg, row_out;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_TEXTSET *T_copy = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;
	char *ptr = NULL;
	void *dest = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_textset: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if ((item = gmtapi_validate_id (API, GMT_IS_TEXTSET, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))	/* Only allow writing of a data set once, unless overridden by mode */
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;
	default_method = GMT_IS_FILE;
	if (S_obj->filename)	/* Write to this file */
		dest = S_obj->filename;
	else {			/* No filename so we switch to writing to the stream */
		default_method = (S_obj->method == GMT_IS_FILE) ? GMT_IS_STREAM : S_obj->method;
		dest = S_obj->fp;
	}
	T_obj->io_mode = mode;
	switch (S_obj->method) {	/* File, array, stream etc ? */
	 	case GMT_IS_STREAM:
#ifdef SET_IO_MODE
			gmt_setmode (GMT, GMT_OUT);
#endif
		case GMT_IS_FILE:
	 	case GMT_IS_FDESC:
			/* gmtlib_write_textset (or lower) will report where it is reading from if level is GMT_MSG_LONG_VERBOSE */
			if ((error = gmtlib_write_textset (GMT, dest, default_method, T_obj, GMT_NOTSET))) return (gmtapi_report_error (API, error));
			break;
		case GMT_IS_DUPLICATE:		/* Duplicate the input textset */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating data table to GMT_TEXTSET memory location\n");
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			T_copy = gmtlib_duplicate_textset (GMT, T_obj, GMT_ALLOC_NORMAL);
			S_obj->resource = T_copy;	/* Set resource pointer from object to this textset */
			break;
		case GMT_IS_REFERENCE:	/* Just pass memory location */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing data table to GMT_TEXTSET memory location\n");
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource must be NULL */
			T_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			S_obj->resource = T_obj;		/* Set resource pointer from object to this textset */
			break;
		case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:
		case GMT_IS_REFERENCE|GMT_VIA_MATRIX:	/* Probably never tested [PW] */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating text table to user array location\n");
			M_obj = gmtlib_duplicate_matrix (GMT, S_obj->resource, false);
			/* Must allocate output space */
			M_obj->n_rows = (GMT->current.io.multi_segments[GMT_OUT]) ? T_obj->n_records + T_obj->n_segments : T_obj->n_records;	/* Number of rows needed [plus segment headers if active] */
			M_obj->n_columns = 1U;		/* Number of columns needed to hold the data records */
			S_obj->n_alloc = gmt_M_get_nm (GMT, M_obj->n_rows, M_obj->dim);	/* Get total number of elements as n_rows * dim */
			if ((error = gmtlib_alloc_univector (GMT, &(M_obj->data), M_obj->type, S_obj->n_alloc)) != GMT_OK) return (gmtapi_report_error (API, error));
			M_obj->alloc_mode = GMT_ALLOC_INTERNALLY;

			ptr = (char *)M_obj->data.sc1;
			for (tbl = row_out = 0; tbl < T_obj->n_tables; tbl++) {
				for (seg = 0; seg < T_obj->table[tbl]->n_segments; seg++) {
					if (GMT->current.io.multi_segments[GMT_OUT]) {
						strncpy (&ptr[row_out*M_obj->dim], T_obj->table[tbl]->segment[seg]->header, M_obj->dim);
						row_out++;
					}
					for (row = 0; row < T_obj->table[tbl]->segment[seg]->n_rows; row++, row_out++)
						strncpy (&ptr[row_out*M_obj->dim], T_obj->table[tbl]->segment[seg]->data[row], M_obj->dim);
				}
			}
			assert (M_obj->n_rows == row_out);	/* Sanity check */
			M_obj->alloc_level = S_obj->alloc_level;
			S_obj->resource = M_obj;		/* Set resource pointer from object to this matrix */
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export text tables\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}
	S_obj->alloc_mode = T_obj->alloc_mode;	/* Clarify allocation mode for this entity */
	S_obj->status = GMT_IS_USED;	/* Mark as read */
	S_obj->data = NULL;

	return GMT_OK;
}

/*! . */
GMT_LOCAL struct GMT_IMAGE *api_import_image (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_IMAGE *image) {
	/* Handles the reading of a 2-D grid given in one of several ways.
	 * Get the entire image:
 	 * 	mode = GMT_CONTAINER_AND_DATA reads both header and image;
	 * Get a subset of the image:  Call api_import_image twice:
	 * 	1. first with mode = GMT_CONTAINER_ONLY which reads header only.  Then, pass
	 *	   the new S_obj-> wesn to match your desired subregion
	 *	2. 2nd with mode = GMT_DATA_ONLY, which reads image based on header's settings
	 * If the image->data array is NULL it will be allocated for you.
	 */

	int item, new_item, new_ID;
	bool done = true, via = false;
	uint64_t i0, i1, j0, j1, ij, ij_orig, row, col;
	unsigned int both_set = (GMT_CONTAINER_ONLY | GMT_DATA_ONLY);
	double dx, dy, d;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_getfunction api_get_val = NULL;
	struct GMT_IMAGE *I_obj = NULL, *I_orig = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;
#ifdef HAVE_GDAL
	bool new = false;
	size_t size;
#endif

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_image: Passed ID = %d and mode = %d\n", object_ID, mode);

	if ((item = gmtapi_validate_id (API, GMT_IS_IMAGE, object_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET) return_null (API, API->error);

	S_obj = API->object[item];		/* Current data object */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))
		return_null (API, GMT_READ_ONCE);	/* Already read this resources before, so fail unless overridden by mode */
	if ((mode & both_set) == both_set) mode -= both_set;	/* Allow users to have set GMT_CONTAINER_ONLY | GMT_DATA_ONLY; reset to GMT_CONTAINER_AND_DATA */

	switch (S_obj->method) {
		case GMT_IS_FILE:	/* Name of a image file on disk */
#ifdef HAVE_GDAL
			if (image == NULL) {	/* Only allocate image struct when not already allocated */
				if (mode & GMT_DATA_ONLY) return_null (API, GMT_NO_GRDHEADER);		/* For mode & GMT_DATA_ONLY grid must already be allocated */
				I_obj = gmtlib_create_image (GMT);
				new = true;
			}
			else
				I_obj = image;	/* We are passing in a image already allocated */
			I_obj->header->complex_mode = mode;		/* Pass on any bitflags */
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read grid */
			if (! (mode & GMT_DATA_ONLY)) {		/* Must init header and read the header information from file */
				if (gmt_M_err_pass (GMT, gmtlib_read_image_info (GMT, S_obj->filename, I_obj), S_obj->filename)) {
					if (new) gmtlib_free_image (GMT, &I_obj, false);
					return_null (API, GMT_IMAGE_READ_ERROR);
				}
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read the grid data themselves. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we extract the entire file domain */
			if (!I_obj->data) {	/* Array is not allocated yet, do so now. We only expect header (and possibly w/e/s/n subset) to have been set correctly */
				I_obj->data = gmt_M_memory (GMT, NULL, I_obj->header->size * I_obj->header->n_bands, unsigned char);
			}
			else {	/* Already have allocated space; check that it is enough */
				size = api_set_grdarray_size (GMT, I_obj->header, mode, S_obj->wesn);	/* Get array dimension only, which includes padding. DANGER DANGER JL*/
				if (size > I_obj->header->size) return_null (API, GMT_IMAGE_READ_ERROR);
			}
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading image from file %s\n", S_obj->filename);
			if (gmt_M_err_pass (GMT, gmtlib_read_image (GMT, S_obj->filename, I_obj, S_obj->wesn,
				I_obj->header->pad, mode), S_obj->filename))
				return_null (API, GMT_IMAGE_READ_ERROR);
			if (gmt_M_err_pass (GMT, gmtlib_image_BC_set (GMT, I_obj), S_obj->filename))
				return_null (API, GMT_IMAGE_BC_ERROR);	/* Set boundary conditions */
			I_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
#else
			GMT_Report (API, GMT_MSG_NORMAL, "GDAL required to read image from file %s\n", S_obj->filename);
#endif
			break;

	 	case GMT_IS_DUPLICATE:	/* GMT grid and header in a GMT_GRID container object. */
			if ((I_orig = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (image == NULL) {	/* Only allocate when not already allocated */
				if (mode & GMT_DATA_ONLY) return_null (API, GMT_NO_GRDHEADER);		/* For mode & GMT_DATA_ONLY grid must already be allocated */
				I_obj = gmtlib_create_image (GMT);
			}
			else
				I_obj = image;	/* We are passing in an image already */
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read grid */
			if (! (mode & GMT_DATA_ONLY)) {	/* Must init header and copy the header information from the existing grid */
				gmt_M_memcpy (I_obj->header, I_orig->header, 1, struct GMT_GRID_HEADER);
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read grid data. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we use everything passed in */
			if (!I_obj->data) {	/* Array is not allocated, do so now. We only expect header (and possibly subset w/e/s/n) to have been set correctly */
				I_obj->header->size = api_set_grdarray_size (GMT, I_obj->header, mode, S_obj->wesn);	/* Get array dimension only, which may include padding */
				I_obj->data = gmt_M_memory (GMT, NULL, I_obj->header->size * I_obj->header->n_bands, unsigned char);
				if (I_orig->alpha) I_obj->alpha = gmt_M_memory (GMT, NULL, I_obj->header->size , unsigned char);
			}
			I_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			if (!S_obj->region && gmt_grd_pad_status (GMT, I_obj->header, GMT->current.io.pad)) {	/* Want an exact copy with no subset and same padding */
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating image data from GMT_IMAGE memory location\n");
				gmt_M_memcpy (I_obj->data, I_orig->data, I_orig->header->size * I_orig->header->n_bands, char);
				if (I_orig->alpha) gmt_M_memcpy (I_obj->alpha, I_orig->alpha, I_orig->header->size, char);
				break;		/* Done with this image */
			}
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Extracting subset image data from GMT_IMAGE memory location\n");
			/* Here we need to do more work: Either extract subset or add/change padding, or both. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			dx = I_obj->header->inc[GMT_X] * I_obj->header->xy_off;	dy = I_obj->header->inc[GMT_Y] * I_obj->header->xy_off;
			j1 = (uint64_t) gmt_M_grd_y_to_row (GMT, I_obj->header->wesn[YLO]+dy, I_orig->header);
			j0 = (uint64_t) gmt_M_grd_y_to_row (GMT, I_obj->header->wesn[YHI]-dy, I_orig->header);
			i0 = (uint64_t) gmt_M_grd_x_to_col (GMT, I_obj->header->wesn[XLO]+dx, I_orig->header);
			i1 = (uint64_t) gmt_M_grd_x_to_col (GMT, I_obj->header->wesn[XHI]-dx, I_orig->header);
			gmt_M_memcpy (I_obj->header->pad, GMT->current.io.pad, 4, int);	/* Set desired padding */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = gmt_M_ijp (I_orig->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = gmt_M_ijp (I_obj->header, row, col);		/* Position of this (row,col) in output grid organization */
					I_obj->data[ij] = I_orig->data[ij_orig];
					if (I_orig->alpha) I_obj->alpha[ij] = I_orig->alpha[ij_orig];
				}
			}
			break;

	 	case GMT_IS_REFERENCE:	/* GMT image and header in a GMT_IMAGE container object by reference */
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing image data from GMT_IMAGE memory location\n");
			if ((I_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read image */
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_image: Change alloc mode\n");
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_image: Check pad\n");
			if (!api_adjust_grdpadding (I_obj->header, GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend G_obj->data to allow for padding, then rearrange rows, but only if item was allocated by GMT */
			if (I_obj->alloc_mode == GMT_ALLOC_EXTERNALLY) return_null (API, GMT_PADDING_NOT_ALLOWED);
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_image: Add pad\n");
#if 0
			gmt_grd_pad_on (GMT, image, GMT->current.io.pad);
#endif
			if (done && S_obj->region) {	/* Possibly adjust the pad so inner region matches wesn */
				if (S_obj->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
					gmtlib_contract_headerpad (GMT, I_obj->header, S_obj->orig_pad, S_obj->orig_wesn);
					S_obj->reset_pad = I_obj->header->reset_pad = 0;
				}
				if (gmtlib_expand_headerpad (GMT, I_obj->header, S_obj->wesn, S_obj->orig_pad, S_obj->orig_wesn))
					S_obj->reset_pad = I_obj->header->reset_pad = 1;
			}
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_image: Return from GMT_IS_REFERENCE\n");
			break;

	 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* The user's 2-D image array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			I_obj = (image == NULL) ? gmtlib_create_image (GMT) : image;	/* Only allocate when not already allocated */
			I_obj->header->complex_mode = mode;	/* Set the complex mode */
			if (! (mode & GMT_DATA_ONLY)) {
				api_matrixinfo_to_grdheader (GMT, I_obj->header, M_obj);	/* Populate a GRD header structure */
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header */
			}
			I_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			/* Must convert to new array */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Importing image data from user memory location\n");
			gmt_set_grddim (GMT, I_obj->header);	/* Set all dimensions */
			I_obj->data = gmt_M_memory (GMT, NULL, I_obj->header->size, unsigned char);
			GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
			api_get_val = api_select_get_function (API, M_obj->type);
			gmt_M_grd_loop (GMT, I_obj, row, col, ij) {
				ij_orig = GMT_2D_to_index (row, col, M_obj->dim);
				api_get_val (&(M_obj->data), ij_orig, &d);
				I_obj->data[ij] = (char)d;
			}
			new_ID = GMT_Register_IO (API, GMT_IS_IMAGE, GMT_IS_DUPLICATE, S_obj->geometry, GMT_IN, NULL, I_obj);	/* Register a new resource to hold I_obj */
			if ((new_item = gmtapi_validate_id (API, GMT_IS_IMAGE, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
				return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
			API->object[new_item]->data = I_obj;
			API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
			I_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
			via = true;
			if (S_obj->region) {	/* Possibly adjust the pad so inner region matches wesn */
				if (S_obj->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
					gmtlib_contract_headerpad (GMT, I_obj->header, S_obj->orig_pad, S_obj->orig_wesn);
					S_obj->reset_pad = I_obj->header->reset_pad = 0;
				}
				if (gmtlib_expand_headerpad (GMT, I_obj->header, S_obj->wesn, S_obj->orig_pad, S_obj->orig_wesn))
					S_obj->reset_pad = I_obj->header->reset_pad = 1;
			}
			break;

	 	case GMT_IS_REFERENCE|GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			I_obj = (image == NULL) ? gmtlib_create_image (GMT) : image;	/* Only allocate when not already allocated */
			I_obj->header->complex_mode = mode;	/* Set the complex mode */
			if (! (mode & GMT_DATA_ONLY)) {
				api_matrixinfo_to_grdheader (GMT, I_obj->header, M_obj);	/* Populate a GRD header structure */
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header */
			}
			if (!(M_obj->shape == GMT_IS_ROW_FORMAT && M_obj->type == GMT_FLOAT && M_obj->alloc_mode == GMT_ALLOC_EXTERNALLY && (mode & GMT_GRID_IS_COMPLEX_MASK)))
				return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing image data from user memory location\n");
			I_obj->data = (unsigned char *)(M_obj->data.sc1);
			S_obj->alloc_mode = M_obj->alloc_mode;	/* Pass on allocation mode of matrix */
			I_obj->alloc_mode = M_obj->alloc_mode;
			if (!api_adjust_grdpadding (I_obj->header, GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			if (I_obj->alloc_mode == GMT_ALLOC_EXTERNALLY) return_null (API, GMT_PADDING_NOT_ALLOWED);
			/* Here we extend I_obj->data to allow for padding, then rearrange rows */
			/* gmt_grd_pad_on (GMT, I, GMT->current.io.pad);*/
			new_ID = GMT_Register_IO (API, GMT_IS_IMAGE, GMT_IS_REFERENCE, S_obj->geometry, GMT_IN, NULL, I_obj);	/* Register a new resource to hold I_obj */
			if ((new_item = gmtapi_validate_id (API, GMT_IS_IMAGE, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
				return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
			API->object[new_item]->data = I_obj;
			API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
			I_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
			via = true;
			if (S_obj->region) {	/* Possibly adjust the pad so inner region matches wesn */
				if (S_obj->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
					gmtlib_contract_headerpad (GMT, I_obj->header, S_obj->orig_pad, S_obj->orig_wesn);
					S_obj->reset_pad = I_obj->header->reset_pad = 0;
				}
				if (gmtlib_expand_headerpad (GMT, I_obj->header, S_obj->wesn, S_obj->orig_pad, S_obj->orig_wesn))
					S_obj->reset_pad = I_obj->header->reset_pad = 1;
			}
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import image\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}

	if (done) S_obj->status = GMT_IS_USED;	/* Mark as read (unless we just got the header) */
	if (!via) S_obj->data = I_obj;		/* Retain pointer to the allocated data so we use garbage collection later */
#ifdef GMT_BACKWARDS_API
	I_obj->ColorMap = I_obj->colormap;
	I_obj->nIndexedColors = I_obj->n_indexed_colors;
#endif

	return ((mode & GMT_DATA_ONLY) ? NULL : I_obj);	/* Pass back out what we have so far */
}

GMT_LOCAL int api_export_ppm (struct GMT_CTRL *GMT, char *fname, struct GMT_IMAGE *I) {
	/* Write a Portable Pixel Map (PPM) file if fname extension is .ppm, else returns */
	//uint32_t row, col, band;
	char *ext = gmt_get_ext (fname), *magic = "P6\n# Produced by GMT\n", dim[GMT_LEN32] = {""};
	FILE *fp = NULL;
	if (strcmp (ext, "ppm")) return 1;	/* Not requesting a PPM file - return 1 and let GDAL take over */

	if ((fp = gmt_fopen (GMT, fname, GMT->current.io.w_mode)) == NULL) {	/* Return -1 to signify failure */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Cannot create file %s\n", fname);
		return -1;
	}
	fwrite (magic, sizeof (char), strlen (magic), fp);	/* Write magic number, linefeed, comment, and another linefeed */
	sprintf (dim, "%d %d\n255\n", I->header->n_rows, I->header->n_columns);
	fwrite (dim, sizeof (char), strlen (dim), fp);	/* Write dimensions and max color value + linefeeds */
	/* Now dump the image in scaneline order, with each pixel as (R, G, B) */
	if (strncmp (I->header->mem_layout, "TRP", 3U)) /* Easy street! */
		fwrite (I->data, sizeof(char), I->header->nm * I->header->n_bands, fp);
#if 0
	else
	for (row = 0; row < I->header->n_rows; row++) {
		for (col = 0; col < I->header->n_columns; col++) {
			for (band = 0; band < I->header->n_bands; band++) {
				fwrite (&(I->data[row+col*I->header->n_rows+band*I->header->nm]), sizeof(char), 1, fp);
			}
		}
	}
#endif
	gmt_fclose (GMT, fp);
	return GMT_NOERROR;
}

/*! Writes out a single image to destination */
GMT_LOCAL int api_export_image (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_IMAGE *I_obj) {
	int item, error;
	bool done = true;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_IMAGE *I_copy = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_image: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if (I_obj->data == NULL && !(mode & GMT_CONTAINER_ONLY)) return (gmtapi_report_error (API, GMT_PTR_IS_NULL));
	if ((item = gmtapi_validate_id (API, GMT_IS_IMAGE, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* The current object whose data we will export */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));	/* Only allow writing of a data set once, unless overridden by mode */
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;
	switch (S_obj->method) {
		case GMT_IS_FILE:	/* Name of an image file on disk */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Writing image to file %s\n", S_obj->filename);
			if ((error = api_export_ppm (API->GMT, S_obj->filename, I_obj)) == 0)
				break;	/* OK, wrote a PPM and we are done */
			else if (error == -1) {	/* Failed to open file */
				GMT_Report (API, GMT_MSG_NORMAL, "Unable to export image\n");
				return (gmtapi_report_error (API, GMT_ERROR_ON_FOPEN));
			}
#ifdef HAVE_GDAL
			else if (gmt_M_err_pass (API->GMT, gmt_export_image (API->GMT, S_obj->filename, I_obj), S_obj->filename))
				return (gmtapi_report_error (API, GMT_IMAGE_WRITE_ERROR));
#else
			else
				GMT_Report (API, GMT_MSG_NORMAL, "GDAL required to write image to file %s\n", S_obj->filename);
#endif
			break;

	 	case GMT_IS_DUPLICATE:	/* Duplicate GMT image to a new GMT_IMAGE container object */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource pointer must be NULL */
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating image data to GMT_IMAGE memory location\n");
			I_copy = GMT_Duplicate_Data (API, GMT_IS_IMAGE, GMT_DUPLICATE_DATA, I_obj);
			S_obj->resource = I_copy;	/* Set resource pointer to the image */
			break;		/* Done with this image */

	 	case GMT_IS_REFERENCE:	/* GMT image and header in a GMT_IMAGE container object - just pass the reference */
			if (S_obj->region) return (gmtapi_report_error (API, GMT_SUBSET_NOT_ALLOWED));
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing image data to GMT_IMAGE memory location\n");
			S_obj->resource = I_obj;	/* Set resource pointer to the image */
			I_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export image\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}

	if (done) S_obj->status = GMT_IS_USED;	/* Mark as written (unless we only updated header) */
	S_obj->data = NULL;

	return (GMT_OK);
}

/*! . */
GMT_LOCAL struct GMT_GRID *api_import_grid (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_GRID *grid) {
	/* Handles the reading of a 2-D grid given in one of several ways.
	 * Get the entire grid:
 	 * 	mode = GMT_CONTAINER_AND_DATA reads both header and grid;
	 * Get a subset of the grid:  Call api_import_grid twice:
	 * 	1. first with mode = GMT_CONTAINER_ONLY which reads header only.  Then, pass
	 *	   the new S_obj-> wesn to match your desired subregion
	 *	2. 2nd with mode = GMT_DATA_ONLY, which reads grid based on header's settings
	 * If the grid->data array is NULL it will be allocated for you.
	 */

	int item, new_item, new_ID;
	bool done = true, new = false, row_by_row, via = false;
 	uint64_t row, col, i0, i1, j0, j1, ij, ij_orig;
	size_t size;
	unsigned int both_set = (GMT_CONTAINER_ONLY | GMT_DATA_ONLY);
	double dx, dy, d;
	p_func_uint64_t GMT_2D_to_index = NULL;
	struct GMT_GRID *G_obj = NULL, *G_orig = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;
	GMT_getfunction api_get_val = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_import_grid: Passed ID = %d and mode = %d\n", object_ID, mode);

	if ((item = gmtapi_validate_id (API, GMT_IS_GRID, object_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET) return_null (API, API->error);

	S_obj = API->object[item];		/* Current data object */
	if (S_obj->status != GMT_IS_UNUSED && S_obj->method == GMT_IS_FILE && !(mode & GMT_IO_RESET)) return_null (API, GMT_READ_ONCE);	/* Already read this file before, so fail unless overridden by mode */
	if ((mode & both_set) == both_set) mode -= both_set;	/* Allow users to have set GMT_CONTAINER_ONLY | GMT_DATA_ONLY; reset to GMT_CONTAINER_AND_DATA */
	row_by_row = ((mode & GMT_GRID_ROW_BY_ROW) || (mode & GMT_GRID_ROW_BY_ROW_MANUAL));
	if (row_by_row && S_obj->method != GMT_IS_FILE) {
		GMT_Report (API, GMT_MSG_NORMAL, "Can only use method GMT_IS_FILE when row-by-row reading of grid is selected\n");
		return_null (API, GMT_NOT_A_VALID_METHOD);
	}

	if (S_obj->region && grid) {	/* See if this is really a subset or just the same region as the grid */
		if (grid->header->wesn[XLO] == S_obj->wesn[XLO] && grid->header->wesn[XHI] == S_obj->wesn[XHI] && grid->header->wesn[YLO] == S_obj->wesn[YLO] && grid->header->wesn[YHI] == S_obj->wesn[YHI]) S_obj->region = false;
	}
	switch (S_obj->method) {
		case GMT_IS_FILE:	/* Name of a grid file on disk */
			if (grid == NULL) {	/* Only allocate grid struct when not already allocated */
				if (mode & GMT_DATA_ONLY) return_null (API, GMT_NO_GRDHEADER);		/* For mode & GMT_DATA_ONLY grid must already be allocated */
				G_obj = gmt_create_grid (GMT);
				new = true;
			}
			else
				G_obj = grid;	/* We are working on a grid already allocated */
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read grid */
			if (! (mode & GMT_DATA_ONLY)) {		/* Must init header and read the header information from file */
				if (row_by_row) {	/* Special row-by-row processing mode */
					char r_mode = (mode & GMT_GRID_NO_HEADER) ? 'R' : 'r';
					/* If we get here more than once we only allocate extra once */
					if (G_obj->extra == NULL) G_obj->extra = gmt_M_memory (GMT, NULL, 1, struct GMT_GRID_ROWBYROW);
					if (api_open_grd (GMT, S_obj->filename, G_obj, r_mode, mode)) {	/* Open the grid for incremental row reading */
						if (new) gmt_free_grid (GMT, &G_obj, false);
						return_null (API, GMT_GRID_READ_ERROR);
					}
				}
				else if (gmt_M_err_pass (GMT, gmtlib_read_grd_info (GMT, S_obj->filename, G_obj->header), S_obj->filename)) {
					if (new) gmt_free_grid (GMT, &G_obj, false);
					return_null (API, GMT_GRID_READ_ERROR);
				}
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read the grid data themselves. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we extract the entire file domain */
			size = api_set_grdarray_size (GMT, G_obj->header, mode, S_obj->wesn);	/* Get array dimension only, which includes padding */
			if (!G_obj->data) {	/* Array is not allocated yet, do so now. We only expect header (and possibly w/e/s/n subset) to have been set correctly */
				G_obj->header->size = size;
				G_obj->data = gmt_M_memory_aligned (GMT, NULL, G_obj->header->size, float);
			}
			else {	/* Already have allocated space; check that it is enough */
				if (size > G_obj->header->size) return_null (API, GMT_GRID_READ_ERROR);
			}
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Reading grid from file %s\n", S_obj->filename);
			if (gmt_M_err_pass (GMT, gmtlib_read_grd (GMT, S_obj->filename, G_obj->header, G_obj->data, S_obj->wesn,
							GMT->current.io.pad, mode), S_obj->filename))
				return_null (API, GMT_GRID_READ_ERROR);
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_IN), S_obj->filename))
				return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			G_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			S_obj->resource = G_obj;	/* Set resource pointer to the grid */
			break;

	 	case GMT_IS_DUPLICATE:	/* GMT grid and header in a GMT_GRID container object. */
			if ((G_orig = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (grid == NULL) {	/* Only allocate when not already allocated */
				if (mode & GMT_DATA_ONLY) return_null (API, GMT_NO_GRDHEADER);		/* For mode & GMT_DATA_ONLY grid must already be allocated */
				G_obj = gmt_create_grid (GMT);
			}
			else
				G_obj = grid;	/* We are passing in a grid already */
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read grid */
			if (! (mode & GMT_DATA_ONLY)) {	/* Must init header and copy the header information from the existing grid */
				gmt_M_memcpy (G_obj->header, G_orig->header, 1, struct GMT_GRID_HEADER);
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header, get out of here */
			}
			/* Here we will read grid data. */
			/* To get a subset we use wesn that is not NULL or contain 0/0/0/0.
			 * Otherwise we use everything passed in */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating grid data from GMT_GRID memory location\n");
			if (!G_obj->data) {	/* Array is not allocated, do so now. We only expect header (and possibly subset w/e/s/n) to have been set correctly */
				G_obj->header->size = api_set_grdarray_size (GMT, G_obj->header, mode, S_obj->wesn);	/* Get array dimension only, which may include padding */
				G_obj->data = gmt_M_memory_aligned (GMT, NULL, G_obj->header->size, float);
			}
			G_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			if (!S_obj->region && gmt_grd_pad_status (GMT, G_obj->header, GMT->current.io.pad)) {	/* Want an exact copy with no subset and same padding */
				gmt_M_memcpy (G_obj->data, G_orig->data, G_orig->header->size, float);
				break;		/* Done with this grid */
			}
			/* Here we need to do more work: Either extract subset or add/change padding, or both. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			dx = G_obj->header->inc[GMT_X] * G_obj->header->xy_off;	dy = G_obj->header->inc[GMT_Y] * G_obj->header->xy_off;
			j1 = (unsigned int)gmt_M_grd_y_to_row (GMT, G_obj->header->wesn[YLO]+dy, G_orig->header);
			j0 = (unsigned int)gmt_M_grd_y_to_row (GMT, G_obj->header->wesn[YHI]-dy, G_orig->header);
			i0 = (unsigned int)gmt_M_grd_x_to_col (GMT, G_obj->header->wesn[XLO]+dx, G_orig->header);
			i1 = (unsigned int)gmt_M_grd_x_to_col (GMT, G_obj->header->wesn[XHI]-dx, G_orig->header);
			gmt_M_memcpy (G_obj->header->pad, GMT->current.io.pad, 4, int);	/* Set desired padding */
			/* get stats */
			G_obj->header->z_min = DBL_MAX;
			G_obj->header->z_max = -DBL_MAX;
			G_obj->header->has_NaNs = GMT_GRID_NO_NANS;	/* We are about to check for NaNs and if none are found we retain 1, else 2 */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = gmt_M_ijp (G_orig->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = gmt_M_ijp (G_obj->header, row, col);		/* Position of this (row,col) in output grid organization */
					G_obj->data[ij] = G_orig->data[ij_orig];
					if (gmt_M_is_fnan (G_obj->data[ij]))
						G_obj->header->has_NaNs = GMT_GRID_HAS_NANS;
					else {
						G_obj->header->z_min = MIN (G_obj->header->z_min, G_obj->data[ij]);
						G_obj->header->z_max = MAX (G_obj->header->z_max, G_obj->data[ij]);
					}
				}
			}
			gmt_BC_init (GMT, G_obj->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_IN), "Grid memory"))
				return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			break;

	 	case GMT_IS_REFERENCE:	/* GMT grid and header in a GMT_GRID container object by reference */
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing grid data from GMT_GRID memory location\n");
			if ((G_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			done = (mode & GMT_CONTAINER_ONLY) ? false : true;	/* Not done until we read grid */
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_grid: Change alloc mode\n");
			S_obj->alloc_mode = G_obj->alloc_mode;
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_grid: Check pad\n");
			gmt_BC_init (GMT, G_obj->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_IN), "Grid memory"))
				return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			if (!api_adjust_grdpadding (G_obj->header, GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			/* Here we extend G_obj->data to allow for padding, then rearrange rows */
			if (G_obj->alloc_mode == GMT_ALLOC_EXTERNALLY) return_null (API, GMT_PADDING_NOT_ALLOWED);
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_grid: Add pad\n");
			gmt_grd_pad_on (GMT, G_obj, GMT->current.io.pad);
			if (done && S_obj->region) {	/* Possibly adjust the pad so inner region matches wesn */
				if (S_obj->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
					gmtlib_contract_headerpad (GMT, G_obj->header, S_obj->orig_pad, S_obj->orig_wesn);
					S_obj->reset_pad = G_obj->header->reset_pad = 0;
				}
				if (gmtlib_expand_headerpad (GMT, G_obj->header, S_obj->wesn, S_obj->orig_pad, S_obj->orig_wesn))
					S_obj->reset_pad = G_obj->header->reset_pad = 1;
			}
			GMT_Report (API, GMT_MSG_DEBUG, "api_import_grid: Return from GMT_IS_REFERENCE\n");
			break;

	 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			G_obj = (grid == NULL) ? gmt_create_grid (GMT) : grid;	/* Only allocate when not already allocated */
			G_obj->header->complex_mode = mode;	/* Set the complex mode */
			if (! (mode & GMT_DATA_ONLY)) {
				api_matrixinfo_to_grdheader (GMT, G_obj->header, M_obj);	/* Populate a GRD header structure */
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header */
			}
			G_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			/* Must convert to new array */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Importing grid data from user memory location\n");
			gmt_set_grddim (GMT, G_obj->header);	/* Set all dimensions */
			G_obj->data = gmt_M_memory_aligned (GMT, NULL, G_obj->header->size, float);
			GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
			G_obj->header->z_min = +DBL_MAX;
			G_obj->header->z_max = -DBL_MAX;
			G_obj->header->has_NaNs = GMT_GRID_NO_NANS;	/* We are about to check for NaNs and if none are found we retain 1, else 2 */
			api_get_val = api_select_get_function (API, M_obj->type);
			gmt_M_grd_loop (GMT, G_obj, row, col, ij) {
				ij_orig = GMT_2D_to_index (row, col, M_obj->dim);
				api_get_val (&(M_obj->data), ij_orig, &d);
				G_obj->data[ij] = (float)d;
				if (gmt_M_is_fnan (G_obj->data[ij]))
					G_obj->header->has_NaNs = GMT_GRID_HAS_NANS;
				else {
					G_obj->header->z_min = MIN (G_obj->header->z_min, G_obj->data[ij]);
					G_obj->header->z_max = MAX (G_obj->header->z_max, G_obj->data[ij]);
				}
			}
			gmt_BC_init (GMT, G_obj->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_IN), "Grid memory"))
				return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			new_ID = GMT_Register_IO (API, GMT_IS_GRID, GMT_IS_DUPLICATE, S_obj->geometry, GMT_IN, NULL, G_obj);	/* Register a new resource to hold G_obj */
			if ((new_item = gmtapi_validate_id (API, GMT_IS_GRID, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
				return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
			API->object[new_item]->data = G_obj;
			API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
			G_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
			via = true;
			break;

	 	case GMT_IS_REFERENCE|GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT YET FULLY TESTED] */
			if ((M_obj = S_obj->resource) == NULL) return_null (API, GMT_PTR_IS_NULL);
			if (S_obj->region) return_null (API, GMT_SUBSET_NOT_ALLOWED);
			G_obj = (grid == NULL) ? gmt_create_grid (GMT) : grid;	/* Only allocate when not already allocated */
			G_obj->header->complex_mode = mode;	/* Set the complex mode */
			if (! (mode & GMT_DATA_ONLY)) {
				api_matrixinfo_to_grdheader (GMT, G_obj->header, M_obj);	/* Populate a GRD header structure */
				if (mode & GMT_CONTAINER_ONLY) break;	/* Just needed the header */
			}
			if (!(M_obj->shape == GMT_IS_ROW_FORMAT && M_obj->type == GMT_FLOAT && M_obj->alloc_mode == GMT_ALLOC_EXTERNALLY && (mode & GMT_GRID_IS_COMPLEX_MASK)))
				 return_null (API, GMT_NOT_A_VALID_IO_ACCESS);
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing grid data from user memory location\n");
			G_obj->data = M_obj->data.f4;
			S_obj->alloc_mode = M_obj->alloc_mode;	/* Pass on alloc_mode of matrix */
			G_obj->alloc_mode = M_obj->alloc_mode;
			gmt_BC_init (GMT, G_obj->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_IN), "Grid memory"))
				return_null (API, GMT_GRID_BC_ERROR);	/* Set boundary conditions */
			if (!api_adjust_grdpadding (G_obj->header, GMT->current.io.pad)) break;	/* Pad is correct so we are done */
			if (G_obj->alloc_mode == GMT_ALLOC_EXTERNALLY) return_null (API, GMT_PADDING_NOT_ALLOWED);
			/* Here we extend G_obj->data to allow for padding, then rearrange rows */
			gmt_grd_pad_on (GMT, G_obj, GMT->current.io.pad);
			new_ID = GMT_Register_IO (API, GMT_IS_GRID, GMT_IS_REFERENCE, S_obj->geometry, GMT_IN, NULL, G_obj);	/* Register a new resource to hold G_obj */
			if ((new_item = gmtapi_validate_id (API, GMT_IS_GRID, new_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
				return_null (API, GMT_OBJECT_NOT_FOUND);	/* Some internal error... */
			API->object[new_item]->data = G_obj;
			API->object[new_item]->status = GMT_IS_USED;	/* Mark as read */
			G_obj->alloc_level = API->object[new_item]->alloc_level;	/* Since allocated here */
			via = true;
			if (S_obj->region) {	/* Possibly adjust the pad so inner region matches wesn */
				if (S_obj->reset_pad) {	/* First undo a prior sub-region used with this memory grid */
					gmtlib_contract_headerpad (GMT, G_obj->header, S_obj->orig_pad, S_obj->orig_wesn);
					S_obj->reset_pad = G_obj->header->reset_pad = 0;
				}
				if (gmtlib_expand_headerpad (GMT, G_obj->header, S_obj->wesn, S_obj->orig_pad, S_obj->orig_wesn))
					S_obj->reset_pad = G_obj->header->reset_pad = 1;
			}
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to import grid\n");
			return_null (API, GMT_NOT_A_VALID_METHOD);
			break;
	}
#ifdef GMT_BACKWARDS_API
	G_obj->header->nx = G_obj->header->n_columns;
	G_obj->header->ny = G_obj->header->n_rows;
#endif
	if (mode & GMT_GRID_XY) {	/* Also allocate and initialize the x and y vectors */
		G_obj->x = GMT_Get_Coord (API, GMT_IS_GRID, GMT_X, G_obj);	/* Get array of x coordinates */
		G_obj->y = GMT_Get_Coord (API, GMT_IS_GRID, GMT_Y, G_obj);	/* Get array of y coordinates */
	}

	if (done) S_obj->status = GMT_IS_USED;	/* Mark as read (unless we just got the header) */
	if (!via) S_obj->data = G_obj;		/* Retain pointer to the allocated data so we use garbage collection later */

	return (G_obj);	/* Pass back out what we have so far */
}

/*! Writes out a single grid to destination */
GMT_LOCAL int api_export_grid (struct GMTAPI_CTRL *API, int object_ID, unsigned int mode, struct GMT_GRID *G_obj) {
	int item, error;
	bool done = true, row_by_row;
	uint64_t row, col, i0, i1, j0, j1, ij, ijp, ij_orig;
	size_t size;
	double dx, dy;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_putfunction api_put_val = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_GRID *G_copy = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_CTRL *GMT = API->GMT;

	GMT_Report (API, GMT_MSG_DEBUG, "api_export_grid: Passed ID = %d and mode = %d\n", object_ID, mode);

	if (object_ID == GMT_NOTSET) return (gmtapi_report_error (API, GMT_OUTPUT_NOT_SET));
	if (G_obj->data == NULL && !(mode & GMT_CONTAINER_ONLY)) return (gmtapi_report_error (API, GMT_PTR_IS_NULL));
	if ((item = gmtapi_validate_id (API, GMT_IS_GRID, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	S_obj = API->object[item];	/* The current object whose data we will export */
	if (S_obj->status != GMT_IS_UNUSED && !(mode & GMT_IO_RESET))
		return (gmtapi_report_error (API, GMT_WRITTEN_ONCE));	/* Only allow writing of a data set once, unless overridden by mode */
	if (mode & GMT_IO_RESET) mode -= GMT_IO_RESET;
	row_by_row = ((mode & GMT_GRID_ROW_BY_ROW) || (mode & GMT_GRID_ROW_BY_ROW_MANUAL));
	if (row_by_row && S_obj->method != GMT_IS_FILE) {
		GMT_Report (API, GMT_MSG_NORMAL, "Can only use method GMT_IS_FILE when row-by-row writing of grid is selected\n");
		return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
	}
	if (S_obj->region) {	/* See if this is really a subset or just the same region as the grid */
		if (G_obj->header->wesn[XLO] == S_obj->wesn[XLO] && G_obj->header->wesn[XHI] == S_obj->wesn[XHI] && G_obj->header->wesn[YLO] == S_obj->wesn[YLO] && G_obj->header->wesn[YHI] == S_obj->wesn[YHI]) S_obj->region = false;
	}
	if (mode & GMT_GRID_IS_GEO) gmt_set_geographic (GMT, GMT_OUT);	/* From API to tell grid is geographic */
	gmtlib_grd_set_units (GMT, G_obj->header);	/* Ensure unit strings are set, regardless of destination */
	switch (S_obj->method) {
		case GMT_IS_FILE:	/* Name of a grid file on disk */
			if (mode & GMT_CONTAINER_ONLY) {	/* Update header structure only */
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Updating grid header for file %s\n", S_obj->filename);
				if (row_by_row) {	/* Special row-by-row processing mode */
					char w_mode = (mode & GMT_GRID_NO_HEADER) ? 'W' : 'w';
					/* Since we may get here twice (initial write; later update) we only allocate extra if NULL */
					if (G_obj->extra == NULL) G_obj->extra = gmt_M_memory (GMT, NULL, 1, struct GMT_GRID_ROWBYROW);
					if (api_open_grd (GMT, S_obj->filename, G_obj, w_mode, mode))	/* Open the grid for incremental row writing */
						return (gmtapi_report_error (API, GMT_GRID_WRITE_ERROR));
				}
				else if (gmt_update_grd_info (GMT, NULL, G_obj->header))
					return (gmtapi_report_error (API, GMT_GRID_WRITE_ERROR));
				done = false;	/* Since we are not done with writing */
			}
			else {
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Writing grid to file %s\n", S_obj->filename);
				if (gmt_M_err_pass (GMT, gmtlib_write_grd (GMT, S_obj->filename, G_obj->header, G_obj->data, S_obj->wesn, G_obj->header->pad, mode), S_obj->filename)) return (gmtapi_report_error (API, GMT_GRID_WRITE_ERROR));
			}
			break;

	 	case GMT_IS_DUPLICATE:	/* Duplicate GMT grid and header to a GMT_GRID container object. Subset allowed */
			if (S_obj->resource) return (gmtapi_report_error (API, GMT_PTR_NOT_NULL));	/* The output resource pointer must be NULL */
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Duplicating grid data to GMT_GRID memory location\n");
			if (!S_obj->region) {	/* No subset, possibly same padding */
				G_copy = GMT_Duplicate_Data (API, GMT_IS_GRID, GMT_DUPLICATE_DATA, G_obj);
				if (api_adjust_grdpadding (G_copy->header, GMT->current.io.pad))
					gmt_grd_pad_on (GMT, G_copy, GMT->current.io.pad);
				gmt_BC_init (GMT, G_copy->header);	/* Initialize grid interpolation and boundary condition parameters */
				if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_copy, GMT_OUT), "Grid memory")) return (gmtapi_report_error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
				S_obj->resource = G_copy;	/* Set resource pointer to the grid */
				break;		/* Done with this grid */
			}
			/* Here we need to extract subset, and possibly change padding. */
			/* Get start/stop row/cols for subset (or the entire domain) */
			G_copy = gmt_create_grid (GMT);
			gmt_M_memcpy (G_copy->header, G_obj->header, 1, struct GMT_GRID_HEADER);
			gmt_M_memcpy (G_copy->header->wesn, S_obj->wesn, 4, double);
			dx = G_obj->header->inc[GMT_X] * G_obj->header->xy_off;	dy = G_obj->header->inc[GMT_Y] * G_obj->header->xy_off;
			j1 = (unsigned int) gmt_M_grd_y_to_row (GMT, G_obj->header->wesn[YLO]+dy, G_obj->header);
			j0 = (unsigned int) gmt_M_grd_y_to_row (GMT, G_obj->header->wesn[YHI]-dy, G_obj->header);
			i0 = (unsigned int) gmt_M_grd_x_to_col (GMT, G_obj->header->wesn[XLO]+dx, G_obj->header);
			i1 = (unsigned int) gmt_M_grd_x_to_col (GMT, G_obj->header->wesn[XHI]-dx, G_obj->header);
			gmt_M_memcpy (G_obj->header->pad, GMT->current.io.pad, 4, int);		/* Set desired padding */
			G_copy->header->size = api_set_grdarray_size (GMT, G_obj->header, mode, S_obj->wesn);	/* Get array dimension only, which may include padding */
			G_copy->data = gmt_M_memory_aligned (GMT, NULL, G_copy->header->size, float);
			G_copy->header->z_min = DBL_MAX;	G_copy->header->z_max = -DBL_MAX;	/* Must set zmin/zmax since we are not writing */
			for (row = j0; row <= j1; row++) {
				for (col = i0; col <= i1; col++, ij++) {
					ij_orig = gmt_M_ijp (G_obj->header, row, col);	/* Position of this (row,col) in original grid organization */
					ij = gmt_M_ijp (G_copy->header, row, col);	/* Position of this (row,col) in output grid organization */
					G_copy->data[ij] = G_obj->data[ij_orig];
					if (gmt_M_is_fnan (G_copy->data[ij])) continue;
					/* Update z_min, z_max */
					G_copy->header->z_min = MIN (G_copy->header->z_min, (double)G_copy->data[ij]);
					G_copy->header->z_max = MAX (G_copy->header->z_max, (double)G_copy->data[ij]);
				}
			}
			gmt_BC_init (GMT, G_copy->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_copy, GMT_OUT), "Grid memory")) return (gmtapi_report_error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
			S_obj->resource = G_copy;	/* Set resource pointer to the grid */
			break;

	 	case GMT_IS_REFERENCE:	/* GMT grid and header in a GMT_GRID container object - just pass the reference */
			if (S_obj->region) return (gmtapi_report_error (API, GMT_SUBSET_NOT_ALLOWED));
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing grid data to GMT_GRID memory location\n");
#if 0
			if (api_adjust_grdpadding (G_obj->header, GMT->current.io.pad)) /* PW: 7/5/2016: Cannot do this since any change to region/pad would be overturned */
				gmt_grd_pad_on (GMT, G_obj, GMT->current.io.pad);	/* Adjust pad */
#endif
			gmt_grd_zminmax (GMT, G_obj->header, G_obj->data);	/* Must set zmin/zmax since we are not writing */
			gmt_BC_init (GMT, G_obj->header);	/* Initialize grid interpolation and boundary condition parameters */
			if (gmt_M_err_pass (GMT, gmt_grd_BC_set (GMT, G_obj, GMT_OUT), "Grid memory")) return (gmtapi_report_error (API, GMT_GRID_BC_ERROR));	/* Set boundary conditions */
			S_obj->resource = G_obj;	/* Set resource pointer to the grid */
			G_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			break;

	 	case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT FULLY TESTED] */
			if (S_obj->resource == NULL) return (gmtapi_report_error (API, GMT_PTR_IS_NULL));	/* The output resource pointer cannot be NULL for matrix */
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			M_obj = gmtlib_duplicate_matrix (GMT, S_obj->resource, false);
			api_grdheader_to_matrixinfo (G_obj->header, M_obj);	/* Populate an array with GRD header information */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Exporting grid data to user memory location\n");
			size = gmt_M_get_nm (GMT, G_obj->header->n_columns, G_obj->header->n_rows);
			if ((error = gmtlib_alloc_univector (GMT, &(M_obj->data), M_obj->type, size)) != GMT_OK) return (error);
			M_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
			GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
			api_put_val = api_select_put_function (API, M_obj->type);
			gmt_M_grd_loop (GMT, G_obj, row, col, ijp) {
				ij = GMT_2D_to_index (row, col, M_obj->dim);
				api_put_val (&(M_obj->data), ij, (double)G_obj->data[ijp]);
			}
			M_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			S_obj->resource = M_obj;	/* Set resource pointer to the matrix */
			break;

	 	case GMT_IS_REFERENCE|GMT_VIA_MATRIX:	/* The user's 2-D grid array of some sort, + info in the args [NOT FULLY TESTED] */
			if (S_obj->resource == NULL) return (gmtapi_report_error (API, GMT_PTR_IS_NULL));	/* The output resource pointer cannot be NULL for matrix */
			if (mode & GMT_CONTAINER_ONLY) return (gmtapi_report_error (API, GMT_NOT_A_VALID_MODE));
			if (api_adjust_grdpadding (G_obj->header, GMT->current.io.pad))
				gmt_grd_pad_on (GMT, G_obj, GMT->current.io.pad);	/* Adjust pad */
			M_obj = gmtlib_duplicate_matrix (GMT, S_obj->resource, false);
			if (!(M_obj->shape == GMT_IS_ROW_FORMAT && M_obj->type == GMT_FLOAT && M_obj->alloc_mode == GMT_ALLOC_EXTERNALLY && (mode & GMT_GRID_IS_COMPLEX_MASK)))
				return (gmtapi_report_error (API, GMT_NOT_A_VALID_IO_ACCESS));
			api_grdheader_to_matrixinfo (G_obj->header, M_obj);	/* Populate an array with GRD header information */
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Referencing grid data to user memory location\n");
			M_obj->data.f4 = G_obj->data;
			M_obj->alloc_level = S_obj->alloc_level;	/* Since we are passing it up to the caller */
			S_obj->resource = M_obj;	/* Set resource pointer to the matrix */
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Wrong method used to export grids\n");
			return (gmtapi_report_error (API, GMT_NOT_A_VALID_METHOD));
			break;
	}

	if (done) S_obj->status = GMT_IS_USED;	/* Mark as written (unless we only updated header) */
	S_obj->data = NULL;

	return (GMT_OK);
}

/*! . */
GMT_LOCAL void *api_import_data (struct GMTAPI_CTRL *API, enum GMT_enum_family family, int object_ID, unsigned int mode, void *data) {

	/* Function that will import the data object referred to by the object_ID (or all registered inputs if object_ID == GMT_NOTSET).
	 * This is a wrapper functions for CPT, Dataset, Textset, Grid and Image imports; see the specific functions
	 * for details on the arguments, in particular the mode setting (or see the GMT API documentation).
	 */
	int item, flag = GMT_NOTSET;
	void *new_obj = NULL;

	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);			/* GMT_Create_Session has not been called */
	if (!API->registered[GMT_IN]) return_null (API, GMT_NO_INPUT);		/* No sources registered yet */

	/* Get information about this resource first */
	if (multiple_files_ok (family)) {
		flag = (API->module_input) ? GMTAPI_MODULE_INPUT : GMTAPI_OPTION_INPUT;
	}
	if ((item = gmtapi_validate_id (API, family, object_ID, GMT_IN, flag)) == GMT_NOTSET) return_null (API, API->error);

	switch (family) {
		case GMT_IS_PALETTE:
			new_obj = api_import_palette (API, object_ID, mode);		/* Try to import a CPT */
			break;
		case GMT_IS_DATASET:
			new_obj = api_import_dataset (API, object_ID, mode);		/* Try to import data tables */
			break;
		case GMT_IS_TEXTSET:
			new_obj = api_import_textset (API, object_ID, mode);		/* Try to import text tables */
			break;
		case GMT_IS_GRID:
			new_obj = api_import_grid (API, object_ID, mode, data);	/* Try to import a grid */
			break;
		case GMT_IS_IMAGE:
			new_obj = api_import_image (API, object_ID, mode, data);	/* Try to import a image */
			break;
		case GMT_IS_MATRIX:
			new_obj = api_import_matrix (API, object_ID, mode);		/* Try to import a matrix */
			break;
		case GMT_IS_VECTOR:
			new_obj = api_import_vector (API, object_ID, mode);		/* Try to import a vector */
			break;
		case GMT_IS_POSTSCRIPT:
			new_obj = api_import_postscript (API, object_ID, mode);		/* Try to import PS */
			break;
		default:
			API->error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	if (new_obj == NULL) return_null (API, API->error);	/* Return NULL as something went wrong */
	return (new_obj);	/* Successful, return pointer */
}

/*! . */
GMT_LOCAL void *api_get_data (void *V_API, int object_ID, unsigned int mode, void *data) {
	/* Function to import registered data sources directly into program memory as a set (not record-by-record).
	 * data is pointer to an existing grid container when we read a grid in two steps, otherwise use NULL.
	 * ID is the registered resource from which to import.
	 * Return: Pointer to data container, or NULL if there were errors (passed back via API->error).
	 */
	int item, family, flag = GMT_NOTSET;
	bool was_enabled;
	void *new_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);

	/* Determine the item in the object list that matches this ID and direction */
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if (object_ID == GMT_NOTSET) {	/* Must pick up the family from the shelf */
		family = API->shelf;
		API->shelf = GMT_NOTSET;
		if (multiple_files_ok(family)) flag = (API->module_input) ? GMTAPI_MODULE_INPUT : GMTAPI_OPTION_INPUT;
	}
	else
		family = GMT_NOTSET;
	if ((item = gmtapi_validate_id (API, family, object_ID, GMT_IN, flag)) == GMT_NOTSET) {
		return_null (API, API->error);
	}

	was_enabled = API->io_enabled[GMT_IN];
	if (!was_enabled && api_begin_io (API, GMT_IN) != GMT_OK) {	/* Enables data input if not already set and sets access mode */
		return_null (API, API->error);
	}
	API->object[item]->selected = true;	/* Make sure it the requested data set is selected */

	/* OK, try to do the importing */
	if ((new_obj = api_import_data (API, API->object[item]->family, object_ID, mode, data)) == NULL) {
		return_null (API, API->error);
	}

	if (!was_enabled && GMT_End_IO (API, GMT_IN, 0) != GMT_OK) {	/* Disables data input if we had to set it in this function */
		return_null (API, API->error);
	}
#ifdef DEBUG
	api_set_object (API, API->object[item]);
	api_list_objects (API, "api_get_data");
#endif
	return (new_obj);		/* Return pointer to the data container */
}

/*! . */
GMT_LOCAL int api_export_data (struct GMTAPI_CTRL *API, enum GMT_enum_family family, int object_ID, unsigned int mode, void *data) {
	/* Function that will export the single data object referred to by the object_ID as registered by GMT_Register_IO.
	 */
	int error, item;

	if (API == NULL) return (GMT_NOT_A_SESSION);	/* GMT_Create_Session has not been called */
	if (data == NULL) return (GMT_PTR_IS_NULL);		/* Got a NULL data pointer */
	if (!API->registered[GMT_OUT]) return (gmtapi_report_error (API, GMT_NO_OUTPUT));		/* No destination registered yet */

	/* Get information about this resource first */
	if ((item = gmtapi_validate_id (API, family, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	/* The case where object_ID is not set but a virtual (memory) file is found is a special case: we must supply the correct object_ID */
	if (object_ID == GMT_NOTSET && item && API->object[item]->method != GMT_IS_FILE)
		object_ID = API->object[item]->ID;	/* Found virtual file; set actual object_ID */

	/* Check if this is a container passed from the outside to capture output */
	if (API->object[item]->messenger && API->object[item]->data) {	/* Need to destroy the dummy container before passing data out */
		error = api_destroy_data_ptr (API, API->object[item]->actual_family, API->object[item]->data);	/* Do the dirty deed */
		if (error) return error;
		GMT_Report (API, GMT_MSG_DEBUG, "api_export_data: Messenger dummy output container for object %d [item %d] freed and set resource=data=NULL\n", API->object[item]->ID, item);
		API->object[item]->resource = API->object[item]->data = NULL;	/* Since we now have nothing */
		API->object[item]->messenger = false;	/* OK, now clean for output */
	}

#ifdef DEBUG
	api_list_objects (API, "api_export_data-in");
#endif
	switch (family) {
		case GMT_IS_PALETTE:	/* Export a CPT */
			error = api_export_palette (API, object_ID, mode, data);
			break;
		case GMT_IS_DATASET:	/* Export a Data set */
			error = api_export_dataset (API, object_ID, mode, data);
			break;
		case GMT_IS_TEXTSET:	/* Export a Text set */
			error = api_export_textset (API, object_ID, mode, data);
			break;
		case GMT_IS_GRID:	/* Export a GMT grid */
			error = api_export_grid (API, object_ID, mode, data);
			break;
		case GMT_IS_IMAGE:	/* Export a GMT image */
			error = api_export_image (API, object_ID, mode, data);
			break;
		case GMT_IS_POSTSCRIPT:	/* Export PS */
			error = api_export_postscript (API, object_ID, mode, data);
			break;
		case GMT_IS_MATRIX:	/* Export MATRIX */
			error = api_export_matrix (API, object_ID, mode, data);
			break;
		case GMT_IS_VECTOR:	/* Export VECTOR */
			error = api_export_vector (API, object_ID, mode, data);
			break;
		default:
			error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
#ifdef DEBUG
	api_list_objects (API, "api_export_data-out");
#endif
	return (gmtapi_report_error (API, error));	/* Return status */
}

/*! . */
GMT_LOCAL int api_put_data (void *V_API, int object_ID, unsigned int mode, void *data) {
	/* Function to write data directly from program memory as a set (not record-by-record).
	 * We can combine the <register resource - export resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * Here, *data is the pointer to the data object to save (CPT, dataset, textset, Grid)
	 * ID is the registered destination.
	 * While only one output destination is allowed, for DATA|TEXTSETS one can
	 * have the tables and even segments be written to individual files (see the mode
	 * description in the documentation for how to enable this feature.)
	 * Return: false if all is well, true if there was an error (and set API->error).
	 */
	int item;
	bool was_enabled;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (data == NULL) return_error (V_API, GMT_PTR_IS_NULL);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	/* Determine the item in the object list that matches this ID and direction */
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET) return_error (API, API->error);

	was_enabled = API->io_enabled[GMT_OUT];
	if (!was_enabled && api_begin_io (API, GMT_OUT) != GMT_OK) {	/* Enables data output if not already set and sets access mode */
		return_error (API, API->error);
	}
	if (api_export_data (API, API->object[item]->family, object_ID, mode, data) != GMT_OK) return_error (API, API->error);

	if (!was_enabled && GMT_End_IO (API, GMT_OUT, 0) != GMT_OK) {	/* Disables data output if we had to set it in this function */
		return_error (API, API->error);
	}
#ifdef DEBUG
	api_set_object (API, API->object[item]);
	api_list_objects (API, "api_put_data");
#endif
	return (GMT_OK);	/* No error encountered */
}

/*! See if this file has already been registered and used.  If so, do not add it again */
GMT_LOCAL bool api_not_used (struct GMTAPI_CTRL *API, char *name) {
	unsigned int item = 0;
	bool not_used = true;
	while (item < API->n_objects && not_used) {
		if (API->object[item] && API->object[item]->direction == GMT_IN && API->object[item]->status != GMT_IS_UNUSED &&
		    API->object[item]->filename && !strcmp (API->object[item]->filename, name))
			/* Used resource with same name */
			not_used = false;	/* Got item with same name, but used */
		else
			item++;	/* No, keep looking */
	}
	return (not_used);
}

/*! . */
GMT_LOCAL int api_init_import (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int geometry, unsigned int mode, struct GMT_OPTION *head) {
	/* Handle registration of data files given with option arguments and/or stdin as input sources.
	 * These are the possible actions taken:
	 * 1. If (mode | GMT_ADD_FILES_IF_NONE) is true and NO resources have previously been registered, then we scan the option list for files (option == '<' (input)).
	 *    For each file found we register the item as a resource.
	 * 2. If (mode | GMT_ADD_FILES_ALWAYS) is true then we always scan the option list for files (option == '<' (input)).
	 *    For each file found we register the item as a resource.
	 * 3. If (mode & GMT_ADD_STDIO_IF_NONE) is true we will register stdin as an input source only if there are NO input items registered.
	 * 4. If (mode & GMT_ADD_STDIO_ALWAYS) is true we will register stdin as an input source, regardless of other items already registered.
	 */

	int object_ID, first_ID = GMT_NOTSET, item;
 	unsigned int n_reg = 0;
	struct GMT_OPTION *current = NULL;
	double *wesn = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_init_import: Passed family = %s and geometry = %s\n", GMT_family[family], GMT_geometry[api_gmtry(geometry)]);

	if (mode & GMT_ADD_EXISTING)
		n_reg = api_add_existing (API, family, geometry, GMT_IN, &first_ID);

	if ((mode & GMT_ADD_FILES_ALWAYS) || ((mode & GMT_ADD_FILES_IF_NONE))) {	/* Wish to register all command-line file args as sources */
		current = head;
		while (current) {	/* Loop over the list and look for input files */
			if (current->option == GMT_OPT_INFILE && api_not_used (API, current->arg)) {	/* File given, register it if has not already been used */
				if (geometry == GMT_IS_SURFACE) {	/* Grids and images may require a subset */
					if (API->GMT->common.R.active[RSET]) {	/* Global subset may have been specified (it might also match the grid/image domain) */
						wesn = gmt_M_memory (API->GMT, NULL, 4U, double);
						gmt_M_memcpy (wesn, API->GMT->common.R.wesn, 4U, double);
					}
				}
				if ((object_ID = GMT_Register_IO (API, family|GMT_VIA_MODULE_INPUT, GMT_IS_FILE, geometry, GMT_IN, wesn, current->arg)) == GMT_NOTSET)
					return_value (API, API->error, GMT_NOTSET);	/* Failure to register */
				n_reg++;	/* Count of new items registered */
				gmt_M_free (API->GMT, wesn);
				if (first_ID == GMT_NOTSET) first_ID = object_ID;	/* Found our first ID */
				if ((item = gmtapi_validate_id (API, family, object_ID, GMT_IN, GMTAPI_MODULE_INPUT)) == GMT_NOTSET)
					return_value (API, API->error, GMT_NOTSET);	/* Some internal error... */
				API->object[item]->selected = true;	/* We will use this variable to find the files to read later */
			}
			current = current->next;	/* Go to next option */
		}
		GMT_Report (API, GMT_MSG_DEBUG, "api_init_import: Added %d new sources\n", n_reg);
	}

	/* Note that n_reg can have changed if we added file args above */

	if ((mode & GMT_ADD_STDIO_ALWAYS) || ((mode & GMT_ADD_STDIO_IF_NONE) && n_reg == 0)) {	/* Wish to register stdin pointer as a source */
		if ((object_ID = GMT_Register_IO (API, family|GMT_VIA_MODULE_INPUT, GMT_IS_STREAM, geometry, GMT_IN, NULL, API->GMT->session.std[GMT_IN])) == GMT_NOTSET)
			return_value (API, API->error, GMT_NOTSET);	/* Failure to register stdin */
		n_reg++;		/* Add the single item */
		if (first_ID == GMT_NOTSET) first_ID = object_ID;	/* Found our first ID */
		if ((item = gmtapi_validate_id (API, family, object_ID, GMT_IN, GMTAPI_MODULE_INPUT)) == GMT_NOTSET)
			return_value (API, API->error, GMT_NOTSET);	/* Some internal error... */
		API->object[item]->selected = true;	/* We will use this variable to find stdin to read from later */
		GMT_Report (API, GMT_MSG_DEBUG, "api_init_import: Added stdin to registered sources\n");
	}
	return (first_ID);
}

/*! . */
GMT_LOCAL int api_init_export (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int geometry, unsigned int mode, struct GMT_OPTION *head) {
	/* Handle registration of output file given with option arguments and/or stdout as output destinations.
	 * Only a single output may be considered.  These are the possible actions taken:
	 * 1. If (mode | GMT_ADD_FILES_IF_NONE) is true and NO destinations have previously been registered,
	 *    then we scan the option list for files (option == '>' (output)).
	 *    Only one file can be registered as a destination; finding more than one results in an error.
	 * 2. If (mode | GMT_ADD_FILES_ALWAYS) is true then we always scan the option list for files (option == '>' (output)).
	 *    Only one file can be registered as a destination; finding more than one results in an error.
	 * 3. If (mode & GMT_ADD_STDIO_IF_NONE) is true we will register stdout as the only destination if there is NO output item registered.
	 * 4. If (mode & GMT_ADD_STDIO_ALWAYS) is true we will register stdout as an destination,
	 *    and give error if other output items have already been registered.
	 */

	unsigned int n_reg = 0;
	int object_ID = GMT_NOTSET, item;
	struct GMT_OPTION *current = NULL, *out_item = NULL;

	GMT_Report (API, GMT_MSG_DEBUG, "api_init_export: Passed family = %s and geometry = %s\n", GMT_family[family], GMT_geometry[api_gmtry(geometry)]);

	if (mode & GMT_ADD_EXISTING)
		n_reg = api_add_existing (API, family, geometry, GMT_OUT, &object_ID);
	if (n_reg > 1) return_value (API, GMT_ONLY_ONE_ALLOWED, GMT_NOTSET);	/* Only one output allowed */

	if ((mode & GMT_ADD_FILES_ALWAYS) || (mode & GMT_ADD_FILES_IF_NONE)) {	/* Wish to register a single output file arg as destination */
		current = head;
		while (current) {	/* Loop over the list and look for output files */
			if (current->option == GMT_OPT_OUTFILE) {	/* Output file given */
				n_reg++;	/* Count it */
				out_item = current;	/* Remember which one it was for later */
			}
			current = current->next;				/* Go to next option */
		}
		if (n_reg > 1) return_value (API, GMT_ONLY_ONE_ALLOWED, GMT_NOTSET);	/* Only one output allowed */

		if (n_reg == 1 && out_item) {	/* Register the single output file found above */
			if ((object_ID = GMT_Register_IO (API, family, GMT_IS_FILE, geometry, GMT_OUT, NULL, out_item->arg)) == GMT_NOTSET)
				return_value (API, API->error, GMT_NOTSET);	/* Failure to register */
			if ((item = gmtapi_validate_id (API, family, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
				return_value (API, API->error, GMT_NOTSET);	/* Some internal error... */
			API->object[item]->selected = true;
			GMT_Report (API, GMT_MSG_DEBUG, "api_init_export: Added 1 new destination\n");
		}
	}
	/* Note that n_reg may have changed if we added file arg */

	if ((mode & GMT_ADD_STDIO_ALWAYS) && n_reg == 1)
		return_value (API, GMT_ONLY_ONE_ALLOWED, GMT_NOTSET);	/* Only one output destination allowed at once */

	if (n_reg == 0 && ((mode & GMT_ADD_STDIO_ALWAYS) || (mode & GMT_ADD_STDIO_IF_NONE))) {	/* Wish to register stdout pointer as a destination */
		if ((object_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_OUT, NULL, API->GMT->session.std[GMT_OUT])) == GMT_NOTSET)
			return_value (API, API->error, GMT_NOTSET);	/* Failure to register stdout?*/
		if ((item = gmtapi_validate_id (API, family, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
			return_value (API, API->error, GMT_NOTSET);	/* Some internal error... */
		API->object[item]->selected = true;
		GMT_Report (API, GMT_MSG_DEBUG, "api_init_export: Added stdout to registered destinations\n");
		n_reg = 1;	/* Only have one item */
	}
	if (n_reg == 0) return_value (API, GMT_OUTPUT_NOT_SET, GMT_NOTSET);	/* No output set */
	return (object_ID);
}

/*! . */
GMT_LOCAL int api_destroy_image (struct GMTAPI_CTRL *API, struct GMT_IMAGE **I_obj) {
	/* Delete the given image resource */

	if (!(*I_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_image: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*I_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmtlib_free_image (API->GMT, I_obj, true);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_grid (struct GMTAPI_CTRL *API, struct GMT_GRID **G_obj) {
	/* Delete the given grid resource. */

	if (!(*G_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_grid: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*G_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmt_free_grid (API->GMT, G_obj, true);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_dataset (struct GMTAPI_CTRL *API, struct GMT_DATASET **D_obj) {
	/* Delete the given dataset resource. */

	if (!(*D_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_dataset: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*D_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmt_free_dataset (API->GMT, D_obj);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_textset (struct GMTAPI_CTRL *API, struct GMT_TEXTSET **T_obj) {
	/* Delete the given textset resource. */

	if (!(*T_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_textset: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*T_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmtlib_free_textset (API->GMT, T_obj);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_palette (struct GMTAPI_CTRL *API, struct GMT_PALETTE **P_obj) {
	/* Delete the given CPT resource. */

	if (!(*P_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_palette: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*P_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmtlib_free_palette (API->GMT, P_obj);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_postscript (struct GMTAPI_CTRL *API, struct GMT_POSTSCRIPT **P_obj) {
	/* Delete the given GMT_POSTSCRIPT resource. */

	if (!(*P_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_postscript: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*P_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmtlib_free_ps (API->GMT, P_obj);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_matrix (struct GMTAPI_CTRL *API, struct GMT_MATRIX **M_obj) {
	/* Delete the given Matrix resource. */

	if (!(*M_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_matrix: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*M_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmtlib_free_matrix (API->GMT, M_obj, true);
	return GMT_OK;
}

/*! . */
GMT_LOCAL int api_destroy_vector (struct GMTAPI_CTRL *API, struct GMT_VECTOR **V_obj) {
	/* Delete the given Matrix resource. */

	if (!(*V_obj)) {	/* Probably not a good sign */
		GMT_Report (API, GMT_MSG_DEBUG, "api_destroy_vector: Passed NULL pointer - skipped\n");
		return (GMT_PTR_IS_NULL);
	}
	if ((*V_obj)->alloc_level != API->GMT->hidden.func_level) return (GMT_FREE_WRONG_LEVEL);	/* Not the right level */

	gmt_free_vector (API->GMT, V_obj, true);
	return GMT_OK;
}

/*! . */
GMT_LOCAL struct GMTAPI_DATA_OBJECT * api_make_dataobject (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int method, unsigned int geometry, void *resource, unsigned int direction) {
	/* Simply the creation and initialization of this DATA_OBJECT structure */
	struct GMTAPI_DATA_OBJECT *S_obj = gmt_M_memory (API->GMT, NULL, 1, struct GMTAPI_DATA_OBJECT);

	S_obj->family    = S_obj->actual_family = family;	/* At creation we are all equal */
	S_obj->method    = method;
	S_obj->geometry  = geometry;
	S_obj->resource  = resource;
	S_obj->direction = direction;

	return (S_obj);
}

/*! . */
GMT_LOCAL int api_colors2cpt (struct GMTAPI_CTRL *API, char **str, unsigned int *mode) {
	/* Take comma-separated color entries given in lieu of a file and build a linear, discrete CPT.
	 * This may be converted to a continuous CPT if -Z is used by makecpt/grd2cpt.
	 * We check if a color is valid then write the given entries verbatim to the temp file.
	 * Returns GMT_NOTSET on error, 0 if no CPT is created (str presumably held a CPT name) and 1 otherwise.
	*/
	unsigned int pos = 0, z = 0;
	char *pch = NULL, color[GMT_LEN256] = {""}, tmp_file[GMT_LEN64] = "";
	double rgb[4] = {0.0, 0.0, 0.0, 0.0};
	FILE *fp = NULL;

	if (!(pch = strchr (*str, ','))) {	/* No comma so presumably a regular CPT name, but check for single color entry */
		bool gray = true;
		size_t k;
		const size_t s_length = strlen(*str);
		 /* Since "gray" is both a master CPT and a shade we must let the CPT take precedence */
		if (!strcmp (*str, "gray"))
			return (0);
		/* Because gmtlib_is_color cannot uniquely determine what a single number is, check for that separately first. */
		for (k = 0; gray && k < s_length; k++)
			if (!isdigit ((*str)[k])) gray = false;	/* Not just a bunch of integers */
		if (gray) {	/* Must also rule out temporary files like 14334.cpt since the ".cpt" is not present */
			sprintf (tmp_file, "%s.cpt", *str);	/* Try this as a filename */
			if (!gmt_access (API->GMT, tmp_file, F_OK))
				return 0;	/* Probably a process id temp file like 13223.cpt */
		}
		if (!gray && !gmtlib_is_color (API->GMT, *str))	/* Not a single color/shade, skip */
			return (0);
	}

	/* OK, here we need to create the temporary palette file */
	snprintf (tmp_file, GMT_LEN64, "api_colors2cpt_%d.cpt", (int)getpid());
	if ((fp = fopen (tmp_file, "w")) == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "Unable to open file %s file for writing\n", tmp_file);
		return (GMT_NOTSET);
	}

	if ((*mode) & GMT_CPT_CONTINUOUS) {	/* Make a continuous cpt from the colors */
		char last_color[GMT_LEN256] = {""};
		if (!gmt_strtok (*str, ",", &pos, last_color)) {	/* Get 1st color entry */
			GMT_Report (API, GMT_MSG_NORMAL, "Unable to find 1st color entry in: %s\n", *str);
			fclose (fp);
			return (GMT_NOTSET);
		}
		if (gmt_getrgb (API->GMT, last_color, rgb)) {
			GMT_Report (API, GMT_MSG_NORMAL, "Badly formatted color entry: %s\n", color);
			fclose (fp);
			return (GMT_NOTSET);
		}
		while (gmt_strtok (*str, ",", &pos, color)) {	/* Get color entries */
			if (gmt_getrgb (API->GMT, color, rgb)) {
				GMT_Report (API, GMT_MSG_NORMAL, "Badly formatted color entry: %s\n", color);
				fclose (fp);
				return (GMT_NOTSET);
			}
			fprintf (fp, "%d\t%s\t%d\t%s\n", z, last_color, z+1, color);
			strncpy (last_color, color, GMT_LEN256-1);
			z++;	/* Increment z-slice values */
		}
		*mode -= GMT_CPT_CONTINUOUS;	/* Served its purpose */
		if (z == 0) {	/* Needed at least two colors to specify a ramp */
			GMT_Report (API, GMT_MSG_NORMAL, "Cannot make a continuous color ramp from a single color: %s\n", *str);
			fclose (fp);
			return (GMT_NOTSET);
		}
	}
	else {
		while (gmt_strtok (*str, ",", &pos, color)) {	/* Get color entries */
			if (gmt_getrgb (API->GMT, color, rgb)) {
				GMT_Report (API, GMT_MSG_NORMAL, "Badly formatted color entry: %s\n", color);
				fclose (fp);
				return (GMT_NOTSET);
			}
			fprintf (fp, "%d\t%s\t%d\t%s\n", z, color, z+1, color);
			z++;	/* Increment z-slice values */
		}
	}
	fclose (fp);

	GMT_Report (API, GMT_MSG_DEBUG, "Converted %s to CPT %s\n", *str, tmp_file);

	gmt_M_str_free (*str);		/* Because it was allocated with strdup */
	*str = strdup (tmp_file);	/* Pass out the temp file name instead */

	return (1);	/* We replaced the name */
}

/*! . */
GMT_LOCAL int api_destroy_coord (struct GMTAPI_CTRL *API, double **ptr) {
	gmt_M_free (API->GMT, *ptr);
	return GMT_OK;
}

/*! Also called in gmt_init.c and prototyped in gmt_internals.h: */
void gmtapi_garbage_collection (struct GMTAPI_CTRL *API, int level) {
	/* gmtapi_garbage_collection frees all registered memory associated with the
	 * current module level or for the entire session if level == GMT_NOTSET (-1). */

	unsigned int i, j, n_free = 0, u_level = 0;
	int error = GMT_NOERROR;
	void *address = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;

	if (API->n_objects == 0) return;	/* Nothing to do */

	/* Free memory allocated during data registration (e.g., via GMT_Get|Put_Data).
	 * Because gmtapi_unregister_io will delete an object and shuffle
	 * the API->object array, reducing API->n_objects by one we must
	 * be aware that API->n_objects changes in the loop below, hence the while loop */

	i = n_free = 0;
	if (level != GMT_NOTSET) u_level = level;
	while (i < API->n_objects) {	/* While there are more objects to consider */
		S_obj = API->object[i];	/* Shorthand for the the current object */
		if (!S_obj) {		/* Skip empty object [NOTE: Should not happen?] */
			GMT_Report (API, GMT_MSG_NORMAL, "gmtapi_garbage_collection found empty object number %d [Bug?]\n", i++);
			continue;
		}
		if (!(level == GMT_NOTSET || S_obj->alloc_level == u_level)) {	/* Not the right module level (or not end of session yet) */
			if (S_obj->reset_pad && S_obj->no_longer_owner == false) {	/* Temporarily changed pad to access a sub-region of a memory grid - now reset this if still the owner */
				address = (S_obj->data) ? S_obj->data : S_obj->resource;	/* Try to get the data object */
				gmtlib_contract_pad (API->GMT, address, S_obj->family, S_obj->orig_pad, S_obj->orig_wesn);
				S_obj->reset_pad = 0;
			}
			i++;	continue;
		}
		if (!(S_obj->data || S_obj->resource)) {	/* No memory to free (probably freed earlier); handle trashing of empty object after this loop */
			i++;	continue;
		}
		if (S_obj->no_longer_owner) {	/* No memory to free since we passed it on; just NULL the pointers */
			S_obj->data = S_obj->resource = NULL;		/* Since other objects own the data now */
			S_obj->alloc_level = u_level;			/* To ensure it will be Unregistered below */
			S_obj->alloc_mode = GMT_ALLOC_INTERNALLY;	/* To ensure it will be Unregistered below */
			i++;	continue;
		}
		/* Here we will try to free the memory pointed to by S_obj->resource|data */
		GMT_Report (API, GMT_MSG_DEBUG, "gmtapi_garbage_collection: Destroying object: C=%d A=%d ID=%d W=%s F=%s M=%s S=%s P=%" PRIxS " D=%" PRIxS " N=%s\n",
			S_obj->close_file, S_obj->alloc_mode, S_obj->ID, GMT_direction[S_obj->direction],
			GMT_family[S_obj->family], GMT_method[S_obj->method], GMT_status[S_obj->status&2],
			(size_t)S_obj->resource, (size_t)S_obj->data, S_obj->filename);
		if (S_obj->data) {
			address = S_obj->data;	/* Keep a record of what the address was (since S_obj->data will be set to NULL when freed) */
			error = api_destroy_data_ptr (API, S_obj->actual_family, API->object[i]->data);	/* Do the dirty deed */
		}
		else if (S_obj->resource) {
			address = S_obj->resource;	/* Keep a record of what the address was (since S_obj->data will be set to NULL when freed) */
			error = api_destroy_data_ptr (API, S_obj->actual_family, API->object[i]->resource);	/* Do the dirty deed */
		}

		if (error < 0) {	/* Failed to destroy this memory; that cannot be a good thing... */
			GMT_Report (API, GMT_MSG_NORMAL, "gmtapi_garbage_collection failed to destroy memory for object % d [Bug?]\n", i++);
			/* Skip it for now; but this is possibly a fatal error [Bug]? */
		}
		else  {	/* Successfully freed.  See if this address occurs more than once (e.g., both for in and output); if so just set repeated data pointer to NULL */
			S_obj->data = S_obj->resource = NULL;
			for (j = i; j < API->n_objects; j++) {
				if (API->object[j]->data == address)
					API->object[j]->data = NULL;		/* Yes, set to NULL so we don't try to free twice */
				if (API->object[j]->resource == address)
					API->object[j]->resource = NULL;	/* Yes, set to NULL so we don't try to free twice */
			}
			n_free++;	/* Number of freed n_objects; do not increment i since GMT_Destroy_Data shuffled the array */
		}
		i++;	/* Go to next object */
	}
 	if (n_free) GMT_Report (API, GMT_MSG_DEBUG, "GMTAPI_Garbage_Collection freed %d memory objects\n", n_free);

	/* Deallocate all remaining objects associated with NULL pointers (e.g., rec-by-rec i/o or those set to NULL above) set during this module (or session) */
	i = 0;
	while (i < API->n_objects) {	/* While there are more objects to consider */
		S_obj = API->object[i];	/* Shorthand for the the current object */
		if (S_obj && (level == GMT_NOTSET || (S_obj->alloc_level == u_level)))	/* Yes, this object was added at this level, get rid of it; do not increment i */
			gmtapi_unregister_io (API, (int)S_obj->ID, (unsigned int)GMT_NOTSET);	/* This shuffles the object array and reduces n_objects */
		else
			i++;	/* Was allocated higher up, leave alone and go to next */
	}
#ifdef DEBUG
	api_list_objects (API, "GMTAPI_Garbage_Collection");
#endif
}

/*! Determine if resource is a filename and that it has already been registered */
GMT_LOCAL int api_memory_registered (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int direction, void *resource) {
	int object_ID = 0, item;
	unsigned int module_input = (family & GMT_VIA_MODULE_INPUT);	/* Are we dealing with a resource that is a module input? */
	family -= module_input;

	if (family == GMT_IS_COORD) return (GMT_NOTSET);	/* Coordinate arrays cannot be a registered memory resource */
	if ((object_ID = api_decode_id (resource)) == GMT_NOTSET) return (GMT_NOTSET);	/* Not a registered resource */
	if ((item = gmtapi_validate_id (API, family, object_ID, direction, GMT_NOTSET)) == GMT_NOTSET) return (GMT_NOTSET);	/* Not the right attributes */
	if (module_input && direction == GMT_IN) API->object[item]->module_input = true;	/* Flag this object as a module input resource */
	return (object_ID);	/* resource is a registered and valid item */
}

/*! Determine if file contains a netCDF directive to a specific variable, e.g., table.nc?time[2] */
GMT_LOCAL bool api_file_with_netcdf_directive (struct GMTAPI_CTRL *API, const char *file) {
	char *duplicate = NULL, *c = NULL;
	if (!strchr (file, '?')) return false;  /* No question mark found */
	duplicate = strdup (file);              /* Found a ?, duplicate this const char string and chop off the end */
	c = strchr (duplicate, '?');            /* Locate the location of ? again */
	if (c) c[0] = '\0';                     /* Chop off text from ? onwards */
	if (gmt_access (API->GMT, duplicate, F_OK)) {	/* No such file, presumably */
		free (duplicate);
		return false;
	}
	else {	/* Since the file exist we know it is a netCDF directive */
		free (duplicate);
		return true;
	}
}

/* Several lower-level API function are needed in a few other gmt_*.c library codes and are thus NOT local.
 * They are listed here and declared via MSC_EXTERN where they occur:
 *   gmtapi_report_error
 *   gmtapi_validate_id
 *   gmtapi_unregister_io
 *   gmtapi_count_objects
 *   gmtapi_close_grd
 *   gmtapi_create_header_item
 * If DEBUG is defined then these two are also accessible:
 *   api_list_objects
 *   api_set_object
 */


/*! ===>  Error message reporting */

int gmtapi_report_error (void *V_API, int error) {
	/* Write error message to log or stderr, then return error code back.
 	 * All functions can call this, even if API has not been initialized. */
	FILE *fp = NULL;
	bool report;
	char message[GMT_LEN256];
	struct GMTAPI_CTRL *API = api_get_api_ptr (V_API);

	report = (API) ? API->error != API->last_error : true;
	if (report && error != GMT_OK) {	/* Report error */
		if (!API || !API->GMT || (fp = API->GMT->session.std[GMT_ERR]) == NULL) fp = stderr;
		if (API && API->session_tag) {
			snprintf (message, GMT_LEN256, "[Session %s (%d)]: Error returned from GMT API: %s (%d)\n",
				API->session_tag, API->session_ID, gmt_api_error_string[error], error);
			GMT_Message (API, GMT_TIME_NONE, message);
		}
		else
			fprintf (fp, "Error returned from GMT API: %s (%d)\n", gmt_api_error_string[error], error);
	}
	if (API) API->last_error = API->error, API->error = error;	/* Update API error value if API exists */
	return (error);
}

/*! . */
int gmtapi_validate_id (struct GMTAPI_CTRL *API, int family, int object_ID, int direction, int module_input) {
	/* Checks to see if the given object_ID is listed and of the right direction.  If so
 	 * we return the item number; otherwise return GMT_NOTSET and set API->error to the error code.
	 * Note: int arguments MAY be GMT_NOTSET, hence we use signed ints.  If object_ID == GMT_NOTSET
	 * then we only look for TEXTSETS or DATASETS.  Note: module_input controls if we are being very specific
	 * about the type of input resource.  There are module inputs and option inputs. We have:
	 * module_input = GMT_NOTSET [-1]:	Do not use the resource's module_input status in determining the next ID.
	 * module_input = GMTAPI_OPTION_INPUT [0]:	Only validate resources with module_input = false.
	 * module_input = GMTAPI_MODULE_INPUT [1]:	Only validate resources with module_input = true.
	 * Finally, since we allow vectors and matrices to masqerade as DATASETs we check for this and
	 * rebaptize such objects to become GMT_IS_DATASETs. */
	unsigned int i;
	int item;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;

	 /* Search for the object in the active list.  However, if object_ID == GMT_NOTSET we pick the first in that direction */

	for (i = 0, item = GMT_NOTSET; item == GMT_NOTSET && i < API->n_objects; i++) {
		S_obj = API->object[i];	/* Shorthand only */
		if (!S_obj) continue;								/* Empty object */
		if (direction == GMT_IN && S_obj->status != GMT_IS_UNUSED) continue;		/* Already used this input object once */
		if (!(family == GMT_NOTSET || (int)S_obj->family == family)) {		/* Not the required data type; check for exceptions... */
			if (family == GMT_IS_DATASET && (S_obj->family == GMT_IS_VECTOR || S_obj->family == GMT_IS_MATRIX))
				S_obj->family = GMT_IS_DATASET;	/* Vectors or Matrix masquerading as dataset are valid. Change their family here. */
			else	/* We don't like your kind */
				continue;
		}
		if (object_ID == GMT_NOTSET && (int)S_obj->direction == direction) item = i;	/* Pick the first object with the specified direction */
		if (object_ID == GMT_NOTSET && !(S_obj->family == GMT_IS_DATASET || S_obj->family == GMT_IS_TEXTSET)) continue;	/* Must be data/text-set */
		else if (direction == GMT_NOTSET && (int)S_obj->ID == object_ID) item = i;	/* Pick the requested object regardless of direction */
		else if ((int)S_obj->ID == object_ID) item = i;					/* Pick the requested object */
		if (item != GMT_NOTSET && direction == GMT_IN && module_input != GMT_NOTSET) {		/* Must check that object's module_input status matches */
			bool status = (module_input == GMTAPI_MODULE_INPUT) ? true : false;
			if (status != S_obj->module_input) item = GMT_NOTSET;	/* Not the right type of input resource */
		}
	}
	if (item == GMT_NOTSET) { API->error = GMT_NOT_A_VALID_ID; return (GMT_NOTSET); }		/* No such object found */

	/* OK, we found the object; is it the right kind (input or output)? */
	if (direction != GMT_NOTSET && (int)(API->object[item]->direction) != direction) {
		/* Passing an input object but it is listed as output, or vice versa */
		if (direction == GMT_IN)  { API->error = GMT_NOT_INPUT_OBJECT;  return (GMT_NOTSET); }
		if (direction == GMT_OUT) { API->error = GMT_NOT_OUTPUT_OBJECT; return (GMT_NOTSET); }
	}
	/* Here we have been successful in finding the right object */
	return (item);
}

/*! . */
int gmtapi_unregister_io (struct GMTAPI_CTRL *API, int object_ID, unsigned int direction) {
	/* Remove specified object ID from active list of objects */
	int s_item;
	unsigned item;

	if (API == NULL) return (GMT_NOT_A_SESSION);		/* GMT_Create_Session has not been called */
	if (API->n_objects == 0) return (gmtapi_report_error (API, GMT_NO_RESOURCES));	/* There are no known resources yet */

	/* Check if this is a valid ID and matches the direction */
	if ((s_item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, direction, GMT_NOTSET)) == GMT_NOTSET) return (gmtapi_report_error (API, API->error));

	/* OK, now it is safe to remove the object; item >= 0 */

	item = s_item;
	GMT_Report (API, GMT_MSG_DEBUG, "gmtapi_unregister_io: Unregistering object no %d [n_objects = %d]\n", API->object[item]->ID, API->n_objects-1);
 	if (API->object[item]->data) GMT_Report (API, GMT_MSG_DEBUG, "gmtapi_unregister_io: Object no %d has non-NULL data pointer\n", API->object[item]->ID);
 	if (API->object[item]->resource) GMT_Report (API, GMT_MSG_DEBUG, "gmtapi_unregister_io: Object no %d has non-NULL resource pointer\n", API->object[item]->ID);

	if (API->object[item]->method == GMT_IS_FILE) gmt_M_str_free (API->object[item]->filename);	/* Free any strdup-allocated filenames */
	gmt_M_free (API->GMT, API->object[item]);		/* Free the current data object */
	API->n_objects--;				/* Tally of how many data sets are left */
	while (item < API->n_objects) {
		API->object[item] = API->object[item+1];	/* Shuffle pointers down one entry */
		item++;
	}

	/* All active resources are found consecutively from 0 to (API->n_objects-1); those with status == 0 (GMT_IS_UNUSED) are available for use. */
	return GMT_OK;
}

/*! . */
unsigned int gmtapi_count_objects (struct GMTAPI_CTRL *API, enum GMT_enum_family family, unsigned int geometry, unsigned int direction, int *first_ID) {
	/* Count how many data sets of the given family are currently registered and unused for the given direction (GMT_IN|GMT_OUT).
 	 * Also return the ID of the first unused data object for the given direction, geometry, and family (GMT_NOTSET if not found).
	 */
	unsigned int i, n;

	*first_ID = GMT_NOTSET;	/* Not found yet */
	for (i = n = 0; i < API->n_objects; i++) {
		if (!API->object[i]) continue;	 /* A freed object, skip it */
		if (API->object[i]->direction != (enum GMT_enum_std)direction) continue;	  /* Wrong direction */
		if (API->object[i]->geometry  != (enum GMT_enum_geometry)geometry) continue;	  /* Wrong geometry */
		if (API->object[i]->status    != GMT_IS_UNUSED) continue; /* Already used */
		if (family != API->object[i]->family) continue;		  /* Wrong data type */
		n++;	/* Found one that satisfied requirements */
		if (*first_ID == GMT_NOTSET) *first_ID = API->object[i]->ID;
	}
	return (n);
}

/*! . */
char *gmtapi_create_header_item (struct GMTAPI_CTRL *API, unsigned int mode, void *arg) {
	size_t lim;
	char *txt = (mode & GMT_COMMENT_IS_OPTION) ? GMT_Create_Cmd (API, arg) : (char *)arg;
	static char buffer[GMT_BUFSIZ];
	gmt_M_memset (buffer, GMT_BUFSIZ, char);
	if (mode & GMT_COMMENT_IS_TITLE) strcat (buffer, "  Title :");
	if (mode & GMT_COMMENT_IS_COMMAND) {
		strcat (buffer, " Command : ");
		if (strlen(API->GMT->init.module_name) < 500)		/* 500, just to shut up a Coverity issue */
			strcat (buffer, API->GMT->init.module_name);
		strcat (buffer, " ");
	}
    if (mode & GMT_COMMENT_IS_REMARK) strcat (buffer, " Remark : ");
	if (mode & GMT_COMMENT_IS_MULTISEG) strcat (buffer, "> ");
	lim = GMT_BUFSIZ - strlen (buffer) - 1;	/* Max characters left */
	strncat (buffer, txt, lim);
	if (mode & GMT_COMMENT_IS_OPTION) gmt_M_free (API->GMT, txt);
	return (buffer);
}

/*! . */
void gmtapi_close_grd (struct GMT_CTRL *GMT, struct GMT_GRID *G) {
	struct GMT_GRID_ROWBYROW *R = api_get_rbr_ptr (G->extra);	/* Shorthand to row-by-row book-keeping structure */
	gmt_M_free (GMT, R->v_row);
	if (GMT->session.grdformat[G->header->type][0] == 'c' || GMT->session.grdformat[G->header->type][0] == 'n')
		nc_close (R->fid);
	else
		gmt_fclose (GMT, R->fp);
	gmt_M_free (GMT, G->extra);
}

/*========================================================================================================
 *          HERE ARE THE PUBLIC GMT API UTILITY FUNCTIONS, WITH THEIR FORTRAN BINDINGS
 *========================================================================================================
 */

/*! ===>  Create a new GMT Session */

void *GMT_Create_Session (const char *session, unsigned int pad, unsigned int mode, int (*print_func) (FILE *, const char *)) {
	/* Initializes the GMT API for a new session. This is typically called once in a program,
	 * but programs that manage many threads might call it several times to create as many
	 * sessions as needed. [Note: There is of yet no thread support built into the GMT API
	 * but you could still manage many sessions at once].
	 * The session argument is a textstring used when reporting errors or messages from activity
	 *   originating within this session.
	 * Pad sets the default number or rows/cols used for grid padding.  GMT uses 2; users of
	 *   the API may wish to use 0 if they have no need for BCs, etc.
	 * The mode argument is a bitflag that controls a few things [0, or GMT_SESSION_NORMAL]:
	 *   bit 1 (GMT_SESSION_NOEXIT)   means call return and not exit when returning from an error.
	 *   bit 2 (GMT_SESSION_EXTERNAL) means we are called by an external API (e.g., MATLAB, Python).
	 *   bit 3 (GMT_SESSION_COLMAJOR) means the external API uses column-major format (e.g., MATLAB, Fortran) [Default is row-major, i.e., C/C++, Python]
	 *   We reserve the right to add future flags.
	 * We return the pointer to the allocated API structure.
	 * If any error occurs we report the error, set the error code via API->error, and return NULL.
	 * We terminate each session with a call to GMT_Destroy_Session.
	 */

	struct GMTAPI_CTRL *API = NULL;
	static char *unknown = "unknown";
	char *dir = NULL;

	if ((API = calloc (1, sizeof (struct GMTAPI_CTRL))) == NULL) return_null (NULL, GMT_MEMORY_ERROR);	/* Failed to allocate the structure */
	API->verbose = (mode >> 16);	/* Pick up any -V settings from gmt.c */
	API->pad = pad;		/* Preserve the default pad value for this session */
	API->print_func = (print_func == NULL) ? api_print_func : print_func;	/* Pointer to the print function to use in GMT_Message|Report */
	API->do_not_exit = mode & GMT_SESSION_NOEXIT;	/* if set, then api_exit & GMT_exit are simply a return; otherwise they call exit */
	API->external = mode & GMT_SESSION_EXTERNAL;	/* if false|0 then we don't list read and write as modules */
	API->shape = (mode & GMT_SESSION_COLMAJOR) ? GMT_IS_COL_FORMAT : GMT_IS_ROW_FORMAT;		/* if set then we must use column-major format [row-major] */
	API->runmode = mode & GMT_SESSION_RUNMODE;		/* If nonzero we set up modern GMT run-mode, else classic */
	if (API->internal) API->leave_grid_scaled = 1;	/* Do NOT undo grid scaling after write since modules do not reuse grids we save some CPU */
	if (session) {	/* Pick up a tag for this session */
		char *tmptag = strdup (session);
		API->session_tag = strdup (basename (tmptag));	/* Only used in reporting and error messages */
		gmt_M_str_free (tmptag);
	}

	/* Set temp directory used by GMT */

#ifdef WIN32
	if ((dir = getenv ("TEMP")))	/* Standard Windows temp directory designation */
		API->tmp_dir = strdup (dir);
	/* If not found we leave it NULL */
#else
	if ((dir = getenv ("TMPDIR")))	/* Alternate tmp dir for *nix */
		API->tmp_dir = strdup (dir);
	else	/* Set standard temporary directory under *nix */
		API->tmp_dir = strdup ("/tmp");
#endif
	API->PPID = api_get_ppid (API);		/* Save PPID for the rest of the session */

	/* gmt_begin initializes, among other things, the settings in the user's (or the system's) gmt.conf file */
	if (gmt_begin (API, session, pad) == NULL) {		/* Initializing GMT and PSL machinery failed */
		gmt_M_str_free (API);	/* Free API */
		return_null (API, GMT_MEMORY_ERROR);
	}
	GMT_Report (API, GMT_MSG_DEBUG, "GMT_Create_Session initialized GMT structure\n");

	API->n_cores = gmtlib_get_num_processors();	/* Get number of available CPU cores */

	/* Allocate memory to keep track of registered data resources */

	API->n_objects_alloc = GMT_SMALL_CHUNK;	/* Start small; this may grow as more resources are registered */
	API->object = gmt_M_memory (API->GMT, NULL, API->n_objects_alloc, struct GMTAPI_DATA_OBJECT *);

	/* Set the unique Session parameters */

	API->session_ID = GMTAPI_session_counter++;		/* Guarantees each session ID will be unique and sequential from 0 up */
	if (session)
		API->GMT->init.module_name = API->session_tag;	/* So non-modules can report name of program, */
	else
		API->GMT->init.module_name = unknown; /* or unknown */

	api_init_sharedlibs (API);				/* Count how many shared libraries we should know about, and get their names and paths */

	return (API);	/* Pass the structure back out */
}

#ifdef FORTRAN_API
/* Fortran binding [THESE MAY CHANGE ONCE WE ACTUALLY TRY TO USE THESE] */
struct GMTAPI_CTRL *GMT_Create_Session_ (const char *tag, unsigned int *pad, unsigned int *mode, void *print, int len) {
	/* Fortran version: We pass the hidden global GMT_FORTRAN structure */
	return (GMT_Create_Session (tag, *pad, *mode, print));
}
#endif

/*! ===>  Destroy a registered GMT Session */

int GMT_Destroy_Session (void *V_API) {
	/* GMT_Destroy_Session terminates the information for the specified session and frees all memory.
	 * Returns false if all is well and true if there were errors. */

	unsigned int i;
	struct GMTAPI_CTRL *API = api_get_api_ptr (V_API);

	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);	/* GMT_Create_Session has not been called */

	GMT_Report (API, GMT_MSG_DEBUG, "Entering GMT_Destroy_Session\n");
	gmtapi_garbage_collection (API, GMT_NOTSET);	/* Free any remaining memory from data registration during the session */
	api_free_sharedlibs (API);			/* Close shared libraries and free list */

	/* Deallocate all remaining objects associated with NULL pointers (e.g., rec-by-rec i/o) */
	for (i = 0; i < API->n_objects; i++) gmtapi_unregister_io (API, (int)API->object[i]->ID, (unsigned int)GMT_NOTSET);
	gmt_M_free (API->GMT, API->object);
	gmt_end (API->GMT);	/* Terminate GMT machinery */
	gmt_M_str_free (API->session_tag);
	gmt_M_str_free (API->tmp_dir);
	gmt_M_memset (API, 1U, struct GMTAPI_CTRL);	/* Wipe it clean first */
 	gmt_M_str_free (API);	/* Not gmt_M_free since this item was allocated before GMT was initialized */

	return (GMT_OK);
}

#ifdef FORTRAN_API
int GMT_Destroy_Session_ () {
	/* Fortran version: We pass the hidden global GMT_FORTRAN structure*/
	return (GMT_Destroy_Session (GMT_FORTRAN));
}
#endif

/*! . */
GMT_LOCAL int api_encode_id (void *V_API, char *filename, int object_ID) {
	/* Creates a filename with the embedded GMTAPI Object ID.  Space must exist.
	 * Limitation:  object_ID must be <= GMTAPI_MAX_ID */

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);	/* GMT_Create_Session has not been called */
	if (!filename) return_error (V_API, GMT_MEMORY_ERROR);		/* Oops, cannot write to that variable */
	if (object_ID == GMT_NOTSET) return_error (V_API, GMT_NOT_A_VALID_ID);	/* ID is not set yet */
	if (object_ID > GMTAPI_MAX_ID) return_error (V_API, GMT_ID_TOO_LARGE);	/* ID is too large to fit in %06d format below */

	sprintf (filename, "@GMTAPI@-%06d", object_ID);	/* Place the object ID in the special GMT API format */
	return_error (V_API, GMT_OK);	/* No error encountered */
}

/* Data registration: The main reason for data registration is the following:
 * Unlike GMT 4, GMT 5 may be used as modules by another calling program.  In
 * that case, the input data file may not be a file but a memory location (i.e.,
 * a data array).  To allow the program to pass such information we needed a
 * way to abstract things so that the modules have no idea of where things are
 * coming from (and were output is going as well).
 * The API session maintains a single linked linear list of data objects; these
 * objects contain information about all the data resources (sources and destinations)
 * that it has been told about.  Because GMT programs (hence the GMT modules) must
 * be able to find data from stdin, command line files, and command options (e.g.,
 * -Gmyfile.txt) we must be flexible in how things are done.
 *
 * Source registration is done in one of several ways:
 *  1. Call GMT_Register_IO directly and specify the source.  The specifics about the
 *     source will be stored in a new data object which is added to the linked list.
 *     This is what top-level programs must do to allow a GMT module to read via a
 *     memory location.
 *  2. Give file names via the option list (this is what happens when stand-alone
 *     GMT programs process the command line argv list).  Depending on the GMT module,
 *     the module will call GMT_Init_IO to scan for such option arguments and then add
 *     each file found as a new data object.
 *  3. Again, depending on the GMT module, if no unused resources are found, the module
 *     will, via GMT_Init_IO, add stdin as an input resource.  This can be in addition
 *     to any other registered sources, but most often it is added because no other
 *     sources were found.
 *
 * The lower-level GMT i/o machinery will handle complications such as 0 (stdin), 1, or
 * many data files so that the modules themselves simply read the next record with
 * GMT_Get_Record until EOF (as if there is only one input source).  Modules that need
 * to store all the data in memory for further processing will call api_get_data instead,
 * which will return a single entity (grid, dataset, cpt, etc).
 *
 * Destination registration is done in the same way, with the exception that for most
 * modules (those processing data tables, at least), only one output destination (e.g., file)
 * can be specified.  However, data sets such as tables with segments can, via mode
 * options, be specified to be written to separate table files or even segment files.
 * The actual writing is done by lower-level functions so that the GMT modules are simply
 * calling api_put_data (all in one go).  For record-by-record output the modules will use
 * GMT_Put_Record.  This keeps data i/o in the modules uniform and simple across GMT.
 */

 /*! . */
int GMT_Register_IO (void *V_API, unsigned int family, unsigned int method, unsigned int geometry, unsigned int direction, double wesn[], void *resource) {
	/* Adds a new data object to the list of registered objects and returns a unique object ID.
	 * Arguments are as listed for api_Register_Im|Export (); see those for details.
	 * During the registration we make sure files exist and are readable.
	 *
	 * if direction == GMT_IN:
	 * A program uses this routine to pass information about input data to GMT.
	 * family:	Specifies the data type we are trying to import; select one of 6 families:
	 *   GMT_IS_PALETTE:	A GMT_PALETTE structure:
	 *   GMT_IS_DATASET:	A GMT_DATASET structure:
	 *   GMT_IS_TEXTSET:	A GMT_TEXTSET structure:
	 *   GMT_IS_GRID:	A GMT_GRID structure:
	 *   GMT_IS_IMAGE:	A GMT_IMAGE structure:
	 *   GMT_IS_POSTSCRIPT:		A GMT_POSTSCRIPT structure:
	 * method:	Specifies by what method we will import this data set:
	 *   GMT_IS_FILE:	A file name is given via input.  The program will read data from this file
	 *   GMT_IS_STREAM:	A file pointer to an open file is passed via input. --"--
	 *   GMT_IS_FDESC:	A file descriptor to an open file is passed via input. --"--
	 *   GMT_IS_DUPLICATE:	A pointer to a data set to be copied
	 *   GMT_IS_REFERENCE:	A pointer to a data set to be passed as is [we may reallocate sizes only if GMT-allocated]
	 * The following approaches can be added to the method for all but CPT:
	 *   GMT_VIA_MATRIX:	A 2-D user matrix is passed via input as a source for copying.
	 *			The GMT_MATRIX structure must have parameters filled out.
	 *   GMT_VIA_VECTOR:	An array of user column vectors is passed via input as a source for copying.
	 *			The GMT_VECTOR structure must have parameters filled out.
	 * geometry:	One of GMT_IS_{TEXT|POINT|LINE|POLY|SURF} (the last for GMT grids)
	 * input:	Pointer to the source filename, stream, handle, array position, etc.
	 * wesn:	Grid subset defined by 4 doubles; otherwise use NULL
	 * RETURNED:	Unique ID assigned to this input resource, or GMT_NOTSET (-1) if error.
	 *
	 * An error status is returned if problems are encountered via API->error [GMT_OK].
	 *
	 * GMT_IS_GRID & GMT_VIA_MATRIX: Since GMT internally uses floats in C arrangement, anything else will be converted to float.
	 * GMT_IS_DATASET & GMT_VIA_MATRIX: Since GMT internally uses doubles in C arrangement, anything else will be converted to double.
	 *
	 * api_Register_Import will allocate and populate a GMTAPI_DATA_OBJECT structure which
	 * is appended to the data list maintained by the GMTAPI_CTRL API structure.
	 *
	 * if direction == GMT_OUT:
	 * The main program uses this routine to pass information about output data from GMT.
	 * family:	Specifies the data type we are trying to export; select one of:
	 *   GMT_IS_PALETTE:	A GMT_PALETTE structure:
	 *   GMT_IS_DATASET:	A GMT_DATASET structure:
	 *   GMT_IS_TEXTSET:	A GMT_TEXTSET structure:
	 *   GMT_IS_IMAGE:	A GMT_IMAGE structure:
	 *   GMT_IS_GRID:	A GMT_GRID structure:
	 *   GMT_IS_POSTSCRIPT:	A GMT_POSTSCRIPT structure:
	 * method:	Specifies by what method we will export this data set:
	 *   GMT_IS_FILE:	A file name is given via output.  The program will write data to this file
	 *   GMT_IS_STREAM:	A file pointer to an open file is passed via output. --"--
	 *   GMT_IS_FDESC:	A file descriptor to an open file is passed via output. --"--
	 *   GMT_IS_DUPLICATE:	A pointer to a data set to be copied
	 *   GMT_IS_REFERENCE:	A pointer to a data set to be passed as is [we may reallocate sizes only if GMT-allocated]
	 * geometry:	One of GMT_IS_{TEXT|POINT|LINE|POLY|SURF} (the last for GMT grids)
	 * output:	Pointer to the destination filename, stream, handle, array position, etc.
	 * wesn:	Grid subset defined by 4 doubles; otherwise use NULL
	 * RETURNED:	Unique ID assigned to this output resource, or GMT_NOTSET (-1) if error.
	 *
	 * An error status is returned if problems are encountered via API->error [GMT_OK].
	 *
	 * api_Register_Export will allocate and populate a GMTAPI_DATA_OBJECT structure which
	 * is appended to the data list maintained by the GMTAPI_CTRL API structure.
	 */
	int item, object_ID;
	unsigned int module_input, mode = method & GMT_IO_RESET;	/* In case we wish to reuse this resource */
	unsigned int first = 0;
	char message[GMT_LEN256], *file = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_value (V_API, GMT_NOT_A_SESSION, GMT_NOTSET);
	API = api_get_api_ptr (V_API);
	API->error = GMT_OK;	/* Reset in case it has some previous error */
	module_input = (family & GMT_VIA_MODULE_INPUT);	/* Are we registering a resource that is a module input? */
	family -= module_input;
	if (api_validate_geometry (API, family, geometry)) return_value (API, GMT_BAD_GEOMETRY, GMT_NOTSET);

	/* Check if this filename is an embedded API Object ID passed via the filename and of the right kind.  */
	if ((object_ID = api_memory_registered (API, family|module_input, direction, resource)) != GMT_NOTSET)
		return (object_ID);	/* OK, return the object ID */

	if ((object_ID = api_is_registered (API, family, geometry, direction, mode, NULL, resource)) != GMT_NOTSET) {	/* Registered before */
		if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, direction, GMT_NOTSET)) == GMT_NOTSET) return_value (API, API->error, GMT_NOTSET);
		if ((family == GMT_IS_GRID || family == GMT_IS_IMAGE) && !full_region (wesn)) {	/* Update the subset region if given (for grids/images only) */
			S_obj = API->object[item];	/* Use S as shorthand */
			gmt_M_memcpy (S_obj->wesn, wesn, 4, double);
			S_obj->region = true;
		}
		return (object_ID);	/* Already registered so we are done */
	}
	method -= mode;	/* Remove GMT_IO_RESET if it was passed */
	GMT = API->GMT;

	switch (method) {	/* Consider CPT, data, text, and grids, accessed via a variety of methods */
		case GMT_IS_FILE:	/* Registration via a single file name */
			/* No, so presumably it is a regular file name */
			file = strdup (resource);
			if (direction == GMT_IN) {	/* For input we can check if the file exists and can be read. */
				char *p = NULL;
				bool not_url = true;
				if ((family == GMT_IS_GRID || family == GMT_IS_IMAGE) && (p = strchr (file, '='))) *p = '\0';	/* Chop off any =<stuff> for grids and images so access can work */
				else if (family == GMT_IS_IMAGE && (p = strchr (file, '+'))) {
					char *c = strchr (file, '.');	/* The period before an extension */
					 /* PW 1/30/2014: Protect images with band requests, e.g., my_image.jpg+b2 */
					if (c && c < p && p[1] == 'b' && isdigit (p[2])) {
						GMT_Report (API, GMT_MSG_DEBUG, "Truncating +b modifier for image filename %s\n", file);
						*p = '\0';	/* Chop off any +b<band> for images at end of extension so access can work */
					}
					else	/* Make sure p is NULL so we don't restore a character below */
						p = NULL;
				}
				if (family == GMT_IS_GRID || family == GMT_IS_IMAGE)	/* Only grid and images can be URLs so far */
					not_url = !gmtlib_check_url_name (file);
				first = gmt_download_file_if_not_found (API->GMT, file);	/* Deal with downloadable GMT data sets first */
				if (gmt_access (GMT, &file[first], F_OK) && not_url) {	/* For input we can check if the file exists (except if via Web) */
					GMT_Report (API, GMT_MSG_NORMAL, "File %s not found\n", &file[first]);
					gmt_M_str_free (file);
					return_value (API, GMT_FILE_NOT_FOUND, GMT_NOTSET);
				}
				if (gmt_access (GMT, &file[first], R_OK) && not_url) {	/* Found it but we cannot read. */
					GMT_Report (API, GMT_MSG_NORMAL, "Not permitted to read file %s\n", &file[first]);
					gmt_M_str_free (file);
					return_value (API, GMT_BAD_PERMISSION, GMT_NOTSET);
				}
				if (p) p[0] = '=';	/* Restore the extensions */
			}
			else if (resource == NULL) {	/* No file given [should this mean stdin/stdout?] */
				gmt_M_str_free (file);
				return_value (API, GMT_OUTPUT_NOT_SET, GMT_NOTSET);
			}
			/* Create a new data object and initialize variables */
			if ((S_obj = api_make_dataobject (API, family, method, geometry, NULL, direction)) == NULL) {
				gmt_M_str_free (file);
				return_value (API, GMT_MEMORY_ERROR, GMT_NOTSET);	/* No more memory */
			}
			if (strlen (resource)) {
				if (first)	/* Must strip off the beginning of the name */
					S_obj->filename = strdup (&file[first]);
				else
					S_obj->filename = strdup (file);
			}
			gmt_M_str_free (file);
			snprintf (message, GMT_LEN256, "Object ID %%d : Registered %s %s %s as an %s resource with geometry %s [n_objects = %%d]\n", GMT_family[family], GMT_method[method], S_obj->filename, GMT_direction[direction], GMT_geometry[api_gmtry(geometry)]);
			break;

		case GMT_IS_STREAM:	/* Methods that indirectly involve a file */
		case GMT_IS_FDESC:
			if (resource == NULL) {	/* No file given [should this mean stdin/stdout?] */
				return_value (API, GMT_OUTPUT_NOT_SET, GMT_NOTSET);
			}
			if ((S_obj = api_make_dataobject (API, family, method, geometry, NULL, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMT_NOTSET);	/* No more memory */
			}
			S_obj->fp = resource;	/* Pass the stream of fdesc onward */
			snprintf (message, GMT_LEN256, "Object ID %%d : Registered %s %s %" PRIxS " as an %s resource with geometry %s [n_objects = %%d]\n", GMT_family[family], GMT_method[method], (size_t)resource, GMT_direction[direction], GMT_geometry[api_gmtry(geometry)]);
			break;

		case GMT_IS_DUPLICATE:
		case GMT_IS_REFERENCE:
			if (direction == GMT_IN && resource == NULL) {
				return_value (API, GMT_PTR_IS_NULL, GMT_NOTSET);	/* Input registration of memory takes a resource */
			}
			if ((S_obj = api_make_dataobject (API, family, method, geometry, resource, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMT_NOTSET);	/* No more memory */
			}
			snprintf (message, GMT_LEN256, "Object ID %%d : Registered %s %s %" PRIxS " as an %s resource with geometry %s [n_objects = %%d]\n", GMT_family[family], GMT_method[method], (size_t)resource, GMT_direction[direction], GMT_geometry[api_gmtry(geometry)]);
			break;

		case GMT_IS_COORD:	/* Internal registration of coordinate arrays so that GMT_Destroy_Data can free them */
			if ((S_obj = api_make_dataobject (API, family, method, geometry, resource, direction)) == NULL) {
				return_value (API, GMT_MEMORY_ERROR, GMT_NOTSET);	/* No more memory */
			}
			snprintf (message, GMT_LEN256, "Object ID %%d : Registered double array %" PRIxS " as an %s resource [n_objects = %%d]\n", (size_t)resource, GMT_direction[direction]);
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Error in GMT_Register_IO (%s): Unrecognized method %d\n", GMT_direction[direction], method);
			return_value (API, GMT_NOT_A_VALID_METHOD, GMT_NOTSET);
			break;
	}

	if (!full_region (wesn)) {	/* Copy the subset region if it was given (for grids) */
		gmt_M_memcpy (S_obj->wesn, wesn, 4, double);
		S_obj->region = true;
	}

	S_obj->alloc_level = GMT->hidden.func_level;	/* Object was allocated at this module nesting level */
	if (module_input) S_obj->module_input = true;

	/* Here S is not NULL and no errors have occurred (yet) */

	if (direction == GMT_OUT && resource == NULL) S_obj->messenger = true;	/* Output messenger */
	if (method != GMT_IS_COORD) API->registered[direction] = true;	/* We have at least registered one item */
	object_ID = api_add_data_object (API, S_obj);
	GMT_Report (API, GMT_MSG_DEBUG, message, object_ID, API->n_objects);
#ifdef DEBUG
	api_list_objects (API, "GMT_Register_IO");
#endif
	return_value (API, API->error, object_ID);
}

#ifdef FORTRAN_API
int GMT_Register_IO_ (unsigned int *family, unsigned int *method, unsigned int *geometry, unsigned int *direction, double wesn[], void *input) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Register_IO (GMT_FORTRAN, *family, *method, *geometry, *direction, wesn, input));
}
#endif


 /*! . */
int GMT_Get_Family (void *V_API, unsigned int direction, struct GMT_OPTION *head) {
	/* Scan the registered module input|output resources to learn what their family is.
	 * direction:	Either GMT_IN or GMT_OUT
	 * head:	Head of the list of module options
	 *
	 * Returns:	The family value (GMT_IS_DATASET|TEXTSET|CPT|GRID|IMAGE|PS) or GMT_NOTSET if not known
	 */
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_OPTION *current = NULL;
	int item, object_ID, family = GMT_NOTSET, flag = (direction == GMT_IN) ? GMTAPI_MODULE_INPUT : GMT_NOTSET;
	unsigned int n_kinds = 0, k, counter[GMT_N_FAMILIES];
	char desired_option = (direction == GMT_IN) ? GMT_OPT_INFILE : GMT_OPT_OUTFILE;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	gmt_M_memset (counter, GMT_N_FAMILIES, unsigned int);	/* Initialize counter */
	API->error = GMT_OK;	/* Reset in case it has some previous error */

	for (current = head; current; current = current->next) {		/* Loop over the list and look for input files */
		if (current->option != desired_option) continue;				/* Not a module resource argument */
		if ((object_ID = api_decode_id (current->arg)) == GMT_NOTSET) continue;	/* Not a registered resource */
		if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, direction, flag)) == GMT_NOTSET) continue;	/* Not the right attributes */
		counter[(API->object[item]->family)]++;	/* Update counts of this family */
	}
	for (k = 0; k < GMT_N_FAMILIES; k++) {	/* Determine which family we found, if any */
		if (counter[k]) n_kinds++, family = k;
	}
	if (n_kinds != 1) {	/* Could not determine family */
		family = GMT_NOTSET;
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Get_Family: Could not determine family\n");
	}
	else	/* Found a unique family */
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Get_Family: Determined family to be %s\n", GMT_family[family]);
	return (family);
}

#ifdef FORTRAN_API
int GMT_Get_Family_ (unsigned int *direction, struct GMT_OPTION *head) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Family (GMT_FORTRAN, *direction, head));
}
#endif

 /*! . */
int GMT_Init_IO (void *V_API, unsigned int family, unsigned int geometry, unsigned int direction, unsigned int mode, unsigned int n_args, void *args) {
	/* Registers program option file arguments as sources/destinations for the current module.
	 * All modules planning to use std* and/or command-line file args must call GMT_Init_IO to register these resources.
	 * family:	The kind of data (GMT_IS_DATASET|TEXTSET|CPT|GRID|IMAGE|PS)
	 * geometry:	Either GMT_IS_NONE|POINT|LINE|POLYGON|SURFACE
	 * direction:	Either GMT_IN or GMT_OUT
	 * mode:	Bitflags composed of 1 = add command line (option) files, 2 = add std* if no other input/output,
	 *		4 = add std* regardless.  mode must be > 0.
	 * n_args:	Either 0 if we pass linked option structs or argc if we pass argv[]
	 * args:	Either linked list of program option arguments (n_args == 0) or char *argv[].
	 *
	 * Returns:	false if successful, true if error.
	 */
	int object_ID;	/* ID of first object [only for debug purposes - not used in this function; ignore -Wunused-but-set-variable warning */
	struct GMT_OPTION *head = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_OK;	/* Reset in case it has some previous error */
	if (api_validate_geometry (API, family, geometry)) return_error (API, GMT_BAD_GEOMETRY);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (API, GMT_NOT_A_VALID_DIRECTION);
	if (!((mode & GMT_ADD_FILES_IF_NONE) || (mode & GMT_ADD_FILES_ALWAYS) || (mode & GMT_ADD_STDIO_IF_NONE) || (mode & GMT_ADD_STDIO_ALWAYS) || (mode & GMT_ADD_EXISTING))) return_error (API, GMT_NOT_A_VALID_MODE);

	if (n_args == 0) /* Passed the head of linked option structures */
		head = args;
	else		/* Passed argc, argv, likely from Fortran */
		head = GMT_Create_Options (API, n_args, args);
	gmtlib_io_banner (API->GMT, direction);	/* Message for binary i/o */
	if (direction == GMT_IN)
		object_ID = api_init_import (API, family, geometry, mode, head);
	else
		object_ID = api_init_export (API, family, geometry, mode, head);
	GMT_Report (API, GMT_MSG_DEBUG, "GMT_Init_IO: Returned first %s object ID = %d\n", GMT_direction[direction], object_ID);
	return (API->error);
}

#ifdef FORTRAN_API
int GMT_Init_IO_ (unsigned int *family, unsigned int *geometry, unsigned int *direction, unsigned int *mode, unsigned int *n_args, void *args) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Init_IO (GMT_FORTRAN, *family, *geometry, *direction, *mode, *n_args, args));
}
#endif

 /*! . */
int GMT_Begin_IO (void *V_API, unsigned int family, unsigned int direction, unsigned int header) {
	/* Initializes the rec-by-rec i/o mechanism for either input or output (given by direction).
	 * GMT_Begin_IO must be called before any data i/o is allowed.
	 * family:	The family of data must be GMT_IS_DATASET or TEXTSET.
	 * direction:	Either GMT_IN or GMT_OUT.
	 * header:	Either GMT_HEADER_ON|OFF, controls the writing of the table start header info block
	 * Returns:	false if successful, true if error.
	 */
	int error, item;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (V_API, GMT_NOT_A_VALID_DIRECTION);
	if (!multiple_files_ok (family)) return_error (V_API, GMT_NOT_A_VALID_IO_ACCESS);
	API = api_get_api_ptr (V_API);
	API->error = GMT_OK;	/* Reset in case it has some previous error */
	if (!API->registered[direction]) GMT_Report (API, GMT_MSG_DEBUG, "GMT_Begin_IO: Warning: No %s resources registered\n", GMT_direction[direction]);

	GMT = API->GMT;
	/* Must initialize record-by-record machinery for dataset or textset */
	GMT_Report (API, GMT_MSG_DEBUG, "GMT_Begin_IO: Initialize record-by-record access for %s\n", GMT_direction[direction]);
	API->current_item[direction] = -1;	/* api_next_data_object (below) will wind it to the first item >= 0 */
	if ((error = api_next_data_object (API, family, direction))) return_error (API, GMT_NO_RESOURCES);	/* Something went bad */
	item = API->current_item[direction];	/* Next item */
	API->io_mode[direction] = GMTAPI_BY_REC;
	API->io_enabled[direction] = true;	/* OK to access resources */
	GMT->current.io.need_previous = (GMT->common.g.active || GMT->current.io.skip_duplicates);
	GMT->current.io.ogr = GMT_OGR_UNKNOWN;
	GMT->current.io.segment_header[0] = GMT->current.io.record[0] = 0;
	if (direction == GMT_OUT && API->object[item]->messenger && API->object[item]->data) {	/* Need to destroy the dummy container before passing data out */
		if ((error = api_destroy_data_ptr (API, API->object[item]->actual_family, API->object[item]->data)))	/* Do the dirty deed */
			return_error (API,error);
		API->object[item]->resource = API->object[item]->data = NULL;	/* Since we now have nothing */
		API->object[item]->messenger = false;	/* OK, now clean for output */
	}
	GMT_Report (API, GMT_MSG_DEBUG, "GMT_Begin_IO: %s resource access is now enabled [record-by-record]\n", GMT_direction[direction]);
	if (direction == GMT_OUT && header == GMT_HEADER_ON && !GMT->common.b.active[GMT_OUT]) GMT_Put_Record (API, GMT_WRITE_TABLE_START, NULL);	/* Write standard ASCII header block */

	return_error (V_API, GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
int GMT_Begin_IO_ (unsigned int *family, unsigned int *direction, unsigned int *header) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Begin_IO (GMT_FORTRAN, *family, *direction, *header));
}
#endif

GMT_LOCAL int api_end_io_dataset (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *S, unsigned int *item) {
	/* These are the steps we must take to finalize a GMT_DATASET that was written to
	 * record-by-record via GMT_Put_Record.  It needs to set number of records and set
	 * the min/max per segments and table */
	int check, object_ID;
	int64_t *count = API->GMT->current.io.curr_pos[GMT_OUT];		/* Short-hand for counts of tbl, seg, rows */
	struct GMT_DATASET *D = S->resource;
	struct GMT_DATATABLE *T = NULL;
	if (D == NULL || D->table == NULL || D->table[0] == NULL) return GMT_OK;	/* Nothing to work on */

	T = D->table[0];	/* Shorthand to the only table */
	if (count[GMT_SEG] >= 0) {	/* Finalize segment allocations */
		if (!T->segment[count[GMT_SEG]]) T->segment[count[GMT_SEG]] = gmt_M_memory (API->GMT, NULL, 1, struct GMT_DATASEGMENT);
		gmtlib_assign_segment (API->GMT, T->segment[count[GMT_SEG]], count[GMT_ROW], T->n_columns);	/* Allocate and place arrays into segment */
		if (T->segment[count[GMT_SEG]]->header == NULL && API->GMT->current.io.record[0]) T->segment[count[GMT_SEG]]->header = strdup (API->GMT->current.io.record);
		count[GMT_SEG]++;	/* Set final number of segments */
		T->n_segments++;
	}
	/* Realloc final number of segments */
	if (count[GMT_SEG] < (int64_t)T->n_alloc) T->segment = gmt_M_memory (API->GMT, T->segment, T->n_segments, struct GMT_DATASEGMENT *);
	D->n_segments = T->n_segments;
	gmtlib_set_dataset_minmax (API->GMT, D);	/* Update the min/max values for this dataset */
	gmtlib_change_dataset (API->GMT, D);		/* Deal with any -o settings */
	D->n_records = T->n_records = count[GMT_ROW];
	D->alloc_level = S->alloc_level;	/* Since we are passing it up to the caller */
	/* Register this resource */
	if ((object_ID = GMT_Register_IO (API, GMT_IS_DATASET, GMT_IS_REFERENCE, D->geometry, GMT_OUT, NULL, D)) == GMT_NOTSET)
		return_error (API, API->error);		/* Failure to register */
	if ((check = gmtapi_validate_id (API, GMT_IS_DATASET, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, API->error);		/* Failure to validate */
	*item = (unsigned int) check;
	return (GMT_OK);
}

GMT_LOCAL int api_end_io_textset (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *S, unsigned int *item) {
	/* These are the steps we must take to finalize a GMT_TEXTSET that was written to
	 * record-by-record via GMT_Put_Record.  It needs to set number of records and set
	 * the min/max per segments and table */
	int check, object_ID;
	int64_t *count = API->GMT->current.io.curr_pos[GMT_OUT];
	struct GMT_TEXTSET *D = S->resource;
	struct GMT_TEXTTABLE *T = NULL;
	if (D == NULL || D->table == NULL || D->table[0] == NULL) return GMT_OK;	/* Nothing to work on */

	T = D->table[0];	/* Shorthand to the only table */
	if (count[GMT_SEG] >= 0) {	/* Last segment */
		T->segment[count[GMT_SEG]]->data = gmt_M_memory (API->GMT, T->segment[count[GMT_SEG]]->data, count[GMT_ROW], char *);
		T->segment[count[GMT_SEG]]->n_rows = count[GMT_ROW];
		count[GMT_SEG]++;	/* Final number of segments */
		T->n_segments++;
	}
	/* Realloc final number of segments */
	if (count[GMT_SEG] < (int64_t)T->n_alloc) T->segment = gmt_M_memory (API->GMT, T->segment, T->n_segments, struct GMT_TEXTSEGMENT *);
	D->n_segments = T->n_segments;
	D->n_records = T->n_records = count[GMT_ROW];
	D->alloc_level = S->alloc_level;	/* Since we are passing it up to the caller */
#ifdef GMT_BACKWARDS_API
	{	/* Set record pointers to data pointers for backwardness */
		uint64_t seg;
		for (seg = 0; seg < T->n_segments; seg++) T->segment[seg]->record = T->segment[seg]->data;
	}
#endif

	/* Register this resource */
	if ((object_ID = GMT_Register_IO (API, GMT_IS_TEXTSET, GMT_IS_REFERENCE, GMT_IS_NONE, GMT_OUT, NULL, D)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to register */
	if ((check = gmtapi_validate_id (API, GMT_IS_TEXTSET, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to validate */
	*item = (unsigned int) check;
	return (GMT_OK);
}

GMT_LOCAL int api_end_io_matrix (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *S, unsigned int *item) {
	/* These are the steps we must take to finalize a GMT_MATRIX that was written to
	 * record-by-record vai GMT_Put_Record.  It needs to set number of rows and possibly
	 * add leading NaN-record(s) if there were segment headers at the beginning of file. */
	int error = 0, check, object_ID;
	struct GMT_MATRIX *M = S->resource;
	if (S->alloc_mode != GMT_ALLOC_EXTERNALLY && S->n_alloc != S->rec) {	/* Must finalize matrix memory */
		size_t size = S->n_alloc = S->rec;
		size *= M->n_columns;
		if ((error = gmtlib_alloc_univector (API->GMT, &(M->data), M->type, size)) != GMT_OK)
			return_error (API, error);
	}
	M->alloc_level = S->alloc_level;	/* Since we are passing it up to the caller */
	if (S->delay) {	/* Must place delayed NaN record(s) signifying segment header(s) */
		GMT_putfunction api_put_val = api_select_put_function (API, M->type);
		p_func_uint64_t GMT_2D_to_index = NULL;
		uint64_t col, ij;
		GMT_2D_to_index = api_get_2d_to_index (API, GMT_IS_ROW_FORMAT, GMT_GRID_IS_REAL);	/* Can only do row-format until end of this function */
		while (S->delay) {	/* Place delayed NaN-rows(s) up front */
			S->delay--;
			M->n_rows++;	/* Since could not be incremented before M was created */
			for (col = 0; col < M->n_columns; col++) {	/* Place the output items */
				ij = GMT_2D_to_index (S->delay, col, M->dim);
				api_put_val (&(M->data), ij, API->GMT->session.d_NaN);
			}
		}
	}
	if (M->shape == GMT_IS_COL_FORMAT) {	/* Oh no, must do a transpose in place */
		GMT_Report (API, GMT_MSG_DEBUG, "api_end_io_matrix: Must transpose union matrix to GMT_IS_COL_FORMAT arrangement\n");
		gmtlib_union_transpose (API->GMT, &(M->data), M->n_rows, M->n_columns, M->type);
		M->dim = M->n_rows;	/* Since now it is in FORTRAN column format */
	}
	/* Register this resource */
	if ((object_ID = GMT_Register_IO (API, GMT_IS_MATRIX, GMT_IS_REFERENCE, GMT_IS_SURFACE, GMT_OUT, NULL, M)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to register */
	if ((check = gmtapi_validate_id (API, GMT_IS_MATRIX, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to validate */
	*item = (unsigned int) check;
	return (GMT_OK);
}

GMT_LOCAL int api_end_io_vector (struct GMTAPI_CTRL *API, struct GMTAPI_DATA_OBJECT *S, unsigned int *item) {
	/* These are the steps we must take to finalize a GMT_VECTOR that was written to
	 * record-by-record vai GMT_Put_Record.  It needs to set number of rows and possibly
	 * add leading NaN-record(s) if there were segment headers at the beginning of file. */
	int error = 0, check, object_ID;
	struct GMT_VECTOR *V = S->resource;
	if (S->alloc_mode != GMT_ALLOC_EXTERNALLY && S->n_alloc != S->rec) {	/* Must finalize memory */
		S->n_alloc = S->rec;
		if ((error = api_alloc_vectors (API->GMT, V, S->n_alloc)) != GMT_OK)
			return_error (API, error);
	}
	if ((object_ID = GMT_Register_IO (API, GMT_IS_VECTOR, GMT_IS_REFERENCE, S->geometry, GMT_OUT, NULL, V)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to register */
	if ((check = gmtapi_validate_id (API, GMT_IS_VECTOR, object_ID, GMT_OUT, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, API->error);	/* Failure to validate */
	V->alloc_level = S->alloc_level;	/* Since we are passing it up to the caller */
	if (S->delay) {	/* Must place delayed NaN record(s) signifying segment header(s) */
		GMT_putfunction api_put_val = api_select_put_function (API, V->type[0]);	/* Since vectors are probably all the same type */
		uint64_t col;
		while (S->delay) {	/* Place delayed NaN-record(s) as leading rows */
			S->delay--;
			V->n_rows++;	/* Since could not be incremented before V was created */
			for (col = 0; col < V->n_columns; col++) {
				if (col && V->type[col] != V->type[col-1]) api_put_val = api_select_put_function (API, V->type[col]);	/* Update put function */
				api_put_val (&(V->data[col]), S->delay, API->GMT->session.d_NaN);
			}
		}
	}
	*item = (unsigned int) check;
	return (GMT_OK);
}

/*! . */
int GMT_End_IO (void *V_API, unsigned int direction, unsigned int mode) {
	/* Terminates the i/o mechanism for either input or output (given by direction).
	 * GMT_End_IO must be called after all data i/o is completed.
	 * direction:	Either GMT_IN or GMT_OUT
	 * mode:	Either GMT_IO_DONE (nothing), GMT_IO_RESET (let all resources be accessible again), or GMT_IO_UNREG (unreg all accessed resources).
	 * NOTE: 	Mode not yet implemented until we see a use.
	 * Returns:	false if successful, true if error.
	 * For memory output we finalized the container, register it, sets the alloc_level to the calling entity
	 * and pass the resource upwards.
	 */
	unsigned int item = 0;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return_error (V_API, GMT_NOT_A_VALID_DIRECTION);
	if (mode > GMT_IO_UNREG) return_error (V_API, GMT_NOT_A_VALID_IO_MODE);

	API = api_get_api_ptr (V_API);
	gmtlib_free_ogr (API->GMT, &(API->GMT->current.io.OGR), 0);	/* Free segment-related array */
	if (direction == GMT_OUT && API->io_mode[GMT_OUT] == GMTAPI_BY_REC) {
		/* Finalize record-by-record output object dimensions */
		S_obj = API->object[API->current_item[GMT_OUT]];	/* Shorthand for the data object we are working on */
		if (S_obj) {
			S_obj->status = GMT_IS_USED;	/* Done "writing" to this destination */
			if ((S_obj->method == GMT_IS_DUPLICATE || S_obj->method == GMT_IS_REFERENCE)) {	/* Used GMT_Put_Record: Must now realloc dimensions given known sizes */
				int error = GMT_OK;	/* If all goes well */
				if (S_obj->actual_family == GMT_IS_DATASET)			/* Dataset type */
					error = api_end_io_dataset (API, S_obj, &item);
				else if (S_obj->actual_family == GMT_IS_MATRIX)		/* Matrix type */
					error = api_end_io_matrix (API, S_obj, &item);
				else if (S_obj->actual_family == GMT_IS_VECTOR)		/* Vector type */
					error = api_end_io_vector (API, S_obj, &item);
				else if (S_obj->actual_family == GMT_IS_TEXTSET)	/* Textset type */
					error = api_end_io_textset (API, S_obj, &item);
				else	/* Should not get here... */
					error = GMT_NOT_A_VALID_FAMILY;
				if (error) return_error (API, error);	/* Failure to finalize */
				API->object[item]->no_longer_owner = true;	/* Since we passed it via S_obj */
				S_obj->data = NULL;	/* Since S_obj->resources points to it too, and needed for api_retrieve_data to work */
			}
			if (S_obj->close_file) {	/* Close any file that we opened earlier */
				gmt_fclose (API->GMT, S_obj->fp);
				S_obj->close_file = false;
			}
		}
	}
	API->io_enabled[direction] = false;	/* No longer OK to access resources or destinations */
	API->current_rec[direction] = 0;	/* Reset count for next time */
	for (item = 0; item < API->n_objects; item++) {	/* Deselect the used resources */
		if (!API->object[item]) continue;	/* Skip empty object */
		if (API->object[item]->direction != (enum GMT_enum_std)direction) continue;	/* Not the required direction */
		if (API->object[item]->selected) API->object[item]->selected = false;	/* No longer a selected resource */
	}

	GMT_Report (API, GMT_MSG_DEBUG, "GMT_End_IO: %s resource access is now disabled\n", GMT_direction[direction]);

	return_error (V_API, GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
int GMT_End_IO_ (unsigned int *direction, unsigned int *mode) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_End_IO (GMT_FORTRAN, *direction, *mode));
}
#endif

/*! . */
int GMT_Status_IO (void *V_API, unsigned int mode) {
	/* Returns nonzero (true) or 0 (false) if the current io status
	 * associated with record-by-record reading matches the
	 * specified mode.  The modes are:
	 * GMT_IO_TABLE_HEADER		: Is current record a table header?
	 * GMT_IO_SEGMENT_HEADER		: Is current record a segment header?
	 * GMT_IO_ANY_HEADER		: Is current record a header or segment header?
	 * GMT_IO_MISMATCH		: Did current record result in a parsing error?
	 * GMT_IO_EOF			: Did we reach end-of-file for entire data set(EOF)?
	 * GMT_IO_NAN			: Did we encounter any NaNs in current record?
	 * GMT_IO_GAP			: Did current record indicate a data gap?
	 * GMT_IO_NEW_SEGMENT		: Is current record the start of a new segment (gap or header)
	 * GMT_IO_LINE_BREAK		: Any sort of new line break (gap, headers, nan)
	 * GMT_IO_FILE_BREAK		: Did we reach end-of-file for a single table (EOF)?
	 * GMT_IO_DATA			: Is current record a data record (including nans)?
	 */

	struct GMTAPI_CTRL *API = NULL;
	struct GMT_IO *IO = NULL;

	if (V_API == NULL) return_value (V_API, GMT_NOT_A_SESSION, GMT_NOTSET);

	API = api_get_api_ptr (V_API);
	IO = &(API->GMT->current.io);	/* Pointer to the GMT IO structure */
	if (mode == GMT_IO_DATA_RECORD) return (IO->status == 0 || IO->status == GMT_IO_NAN);
	return (IO->status & mode);
}

#ifdef FORTRAN_API
int GMT_Status_IO_ (unsigned int *mode) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Status_IO (GMT_FORTRAN, *mode));
}
#endif

/*! . */
GMT_LOCAL int api_get_id (void *V_API, unsigned int family, unsigned int direction, void *resource) {
	unsigned int i;
	int item;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);	/* GMT_Create_Session has not been called */
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	for (i = 0, item = GMT_NOTSET; item == GMT_NOTSET && i < API->n_objects; i++) {
		if (!API->object[i]) continue;				/* Empty object */
		if (!API->object[i]->resource) continue;		/* Empty resource */
		if (API->object[i]->family != (enum GMT_enum_family)family) {		/* Not the required data type, but check for exceptions */
			if (family == GMT_IS_DATASET && (API->object[i]->family == GMT_IS_MATRIX || API->object[i]->family == GMT_IS_VECTOR))
				API->object[i]->family = GMT_IS_DATASET;	/* Vectors or Matrix masquerading as dataset are valid. Change their family here. */
			else
				continue;
		}
		if (API->object[i]->direction != (enum GMT_enum_std)direction) continue;	/* Not the required direction */
		if (API->object[i]->resource == resource) item = i;	/* Pick the requested object regardless of direction */
	}
	if (item == GMT_NOTSET) { API->error = GMT_NOT_A_VALID_ID; return (GMT_NOTSET); }	/* No such resource found */
	return (API->object[item]->ID);
}

 /*! . */
int GMT_Open_VirtualFile (void *V_API, unsigned int family, unsigned int geometry, unsigned int direction, void *data, char *name) {
	/* Associate a virtual file with a data object for either reading or writing.
	 * Family and geometry specifies the nature of the data to be read or written.
	 * Direction is either GMT_IN or GMT_OUT and determines if we read or write.
	 * Reading: data must point to a data container we wish to read from via a module.
	 * Writing: data is either an existing output data container that the user created
	 *  beforehand or it is NULL and we create an expanding output resource.
	 * name is the name given to the virtual file and is returned. */
	int object_ID = GMT_NOTSET, item_s = 0;
	unsigned int item;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (!(direction == GMT_IN || direction == GMT_OUT)) return GMT_NOT_A_VALID_DIRECTION;
	if (direction == GMT_IN && data == NULL) return GMT_PTR_IS_NULL;
	if (name == NULL) return_error (V_API, GMT_PTR_IS_NULL);
	API = api_get_api_ptr (V_API);

	if (data) {	/* Data container provided, see if registered */
		for (item = 0; object_ID == GMT_NOTSET && item < API->n_objects; item++) {	/* Loop over all objects */
			if (!API->object[item]) continue;	/* Skip freed objects */
			if (API->object[item]->data == data) object_ID = API->object[item]->ID;	/* Found a matching data pointer */
		}
		if (object_ID != GMT_NOTSET && (item_s = api_get_item (API, family, data)) == GMT_NOTSET) {	/* Not found in list */
			return_error (API, GMT_OBJECT_NOT_FOUND);	/* Could not find that item in the array despite finding its ID? */
		}
		S_obj = API->object[item_s];	/* Short-hand for later */
	}
	if (direction == GMT_IN) {	/* Set things up for reading */
		/* See if this one is known to us already */
		if (object_ID == GMT_NOTSET) {	/* Register data as a new object for reading [GMT_IN] and reset its status to unread */
			if ((object_ID = GMT_Register_IO (API, family, GMT_IS_REFERENCE|GMT_IO_RESET, geometry, GMT_IN, NULL, data)) == GMT_NOTSET)
				return (API->error);
		}
		else {	/* Found the object earlier; recycle the address and ensure it is a readable object */
			S_obj->status = 0;					/* Open for business */
			S_obj->resource = S_obj->data;		/* Switch from consumer to provider */
			S_obj->data = NULL;					/* No longer consumer */
			S_obj->method = GMT_IS_REFERENCE;	/* Now a memory resource */
			S_obj->direction = GMT_IN;			/* Make sure it now is flagged for reading */
		}
	}
	else {	/* Set things up for writing */
		if (data) {	/* Was provided an object to use */
			if (object_ID == GMT_NOTSET) {	/* Register a new object for writing [GMT_OUT] and reset its status to unread */
				if ((object_ID = GMT_Register_IO (API, family, GMT_IS_REFERENCE|GMT_IO_RESET, geometry, GMT_OUT, NULL, data)) == GMT_NOTSET)
					return (API->error);
			}
			else {	/* Here we have the item and can recycle the address */
				S_obj->status = 0;			/* Open for business */
				if (S_obj->data == NULL) {	/* Switch from provider to consumer, if needed */
					S_obj->data = S_obj->resource;
					S_obj->resource = NULL;		/* No longer consumer */
				}
				S_obj->method = GMT_IS_REFERENCE;	/* Now a memory resource */
				S_obj->direction = GMT_OUT;			/* Make sure it now is flagged for writing */
			}
		}
		else {	/* New expanding output resource */
			void *object = NULL;
			/* GMT_Create_Data may return error code if there are issues with the values of family, or geometry */
			/* Creating an empty object with mode == GMT_IS_OUTPUT means it is intended to hold output [GMT_OUT] from a module */
			if ((object = GMT_Create_Data (API, family, geometry, GMT_IS_OUTPUT, NULL, NULL, NULL, 0, 0, NULL)) == NULL)
				return (API->error);
			/* Obtain the object's ID */
			if ((object_ID = api_get_id (API, family, GMT_OUT, object)) == GMT_NOTSET)
				return (API->error);
		}
	}
	/* Obtain the unique VirtualFile name */
	if (api_encode_id (API, name, object_ID) != GMT_NOERROR)
		return (API->error);
	return GMT_NOERROR;
}

#ifdef FORTRAN_API
int GMT_Open_VirtualFile_ (unsigned int *family, unsigned int *geometry, unsigned int *direction, void *data, char *string, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Open_VirtualFile (GMT_FORTRAN, *family, *geometry, *direction, data, string));
}
#endif

int GMT_Close_VirtualFile (void *V_API, const char *string) {
	/* Given a VirtualFile name, close it */
	int object_ID, item;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (string == NULL) return_error (V_API, GMT_PTR_IS_NULL);
	if ((object_ID = api_decode_id (string)) == GMT_NOTSET)
		return_error (V_API, GMT_OBJECT_NOT_FOUND);
	API = api_get_api_ptr (V_API);
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, GMT_OBJECT_NOT_FOUND);
	S_obj = API->object[item];	/* Short-hand */
	if (S_obj->direction == GMT_IN) {
		S_obj->data = S_obj->resource;	/* Switch from provider to consumer */
		S_obj->resource = NULL;			/* No longer provider */
	}
	if (S_obj->family != S_obj->actual_family)	/* Reset the un-masquerading that GMT_Open_VirtualFile did */
		S_obj->family = S_obj->actual_family;
	return GMT_NOERROR;
}

#ifdef FORTRAN_API
int GMT_Close_VirtualFile_ (unsigned int *family, char *string, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Close_VirtualFile (GMT_FORTRAN, string));
}
#endif

void *GMT_Read_VirtualFile (void *V_API, const char *string) {
	/* Given a VirtualFile name, retrieve the resulting object */
	int object_ID;
	void *object = NULL;
	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	if (string == NULL) return_null (V_API, GMT_PTR_IS_NULL);
	if ((object_ID = api_decode_id (string)) == GMT_NOTSET)
		return_null (V_API, GMT_OBJECT_NOT_FOUND);
	if ((object = api_retrieve_data (V_API, object_ID)) == NULL)
		return_null (V_API, GMT_OBJECT_NOT_FOUND);
	return object;
}

#ifdef FORTRAN_API
void *GMT_Read_VirtualFile_ (char *string, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Read_VirtualFile (GMT_FORTRAN, string));
}
#endif

 /*! . */
int GMT_Init_VirtualFile (void *V_API, unsigned int mode, const char *name) {
	/* Reset a virtual file back to its original configuration so that it can be
	 * repurposed for reading or writing again.
	 */
	int object_ID = GMT_NOTSET, item;
	struct GMTAPI_DATA_OBJECT *S = NULL;
	struct GMTAPI_CTRL *API = NULL;
	gmt_M_unused (mode);

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (name == NULL) return_error (V_API, GMT_PTR_IS_NULL);
	API = api_get_api_ptr (V_API);
	if ((object_ID = api_decode_id (name)) == GMT_NOTSET) return (GMT_OBJECT_NOT_FOUND);	/* Not a registered resource */
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET)
		return_error (API, GMT_OBJECT_NOT_FOUND);
	S = API->object[item];	/* Short-hand pointer */
	S->rec = 0;	/* Start at first record */
	S->delay = 0;	/* No Nan-fuckery yet */
	S->status = GMT_IS_UNUSED;
	S->selected = true;
	return GMT_NOERROR;
}

#ifdef FORTRAN_API
int GMT_Init_VirtualFile_ (unsigned int mode, char *string, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Init_VirtualFile (GMT_FORTRAN, mode, string));
}
#endif

/*! . */
void *GMT_Read_Data (void *V_API, unsigned int family, unsigned int method, unsigned int geometry, unsigned int mode, double wesn[], const char *infile, void *data) {
	/* Function to read data files directly into program memory as a set (not record-by-record).
	 * We can combine the <register resource - import resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * data is pointer to an existing grid container when we read a grid in two steps, otherwise use NULL.
	 * Case 1: infile != NULL: Register input as the source and import data.
	 * Case 2: infile == NULL: Register stdin as the source and import data.
	 * Case 3: geometry == 0: Loop over all previously registered AND unread sources and combine as virtual dataset [DATASET|TEXTSET only]
	 * Case 4: family is GRID|IMAGE and method = GMT_DATA_ONLY: Just find already registered resource
	 * Return: Pointer to data container, or NULL if there were errors (passed back via API->error).
	 */
	int in_ID = GMT_NOTSET, item = GMT_NOTSET;
	unsigned int module_input = 0;
	bool just_get_data, reset;
	void *new_obj = NULL;
	char *input = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	if (infile) input = strdup (infile);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	just_get_data = (gmt_M_file_is_memory (input));	/* A regular GMT resource passed via memory */
	reset = (mode & GMT_IO_RESET);	/* We want to reset resource as unread after reading it */
	if (reset) mode -= GMT_IO_RESET;
	module_input = (family & GMT_VIA_MODULE_INPUT);	/* Are we reading a resource that should be considered a module input? */
	family -= module_input;
	API->module_input = (module_input) ? true : false;
	if (!gmt_M_file_is_cache(infile) && !gmt_M_file_is_url(infile) && infile && strpbrk (infile, "*?[]") && !api_file_with_netcdf_directive (API, infile)) {	/* Gave a wildcard filename */
		uint64_t n_files;
		unsigned int k;
		char **filelist = NULL;
		if (!multiple_files_ok (family)) {
			GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Data: Wildcards only allowed for DATASET and TEXTSET. "
			                                 "Use GMT_Read_Group to read groups of other data types\n");
			free (input);
			return_null (API, GMT_ONLY_ONE_ALLOWED);
		}
		if ((n_files = gmtlib_glob_list (API->GMT, infile, &filelist)) == 0) {
			GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Data: Expansion of \"%s\" gave no results\n", infile);
			free (input);
			return_null (API, GMT_OBJECT_NOT_FOUND);
		}
		API->shelf = family;	/* Save which one it is so we know in api_get_data */
		API->module_input = true;	/* Since we are passing NULL as file name we must loop over registered resources */
		for (k = 0; k < n_files; k++) {
			if ((in_ID = GMT_Register_IO (API, family|GMT_VIA_MODULE_INPUT, GMT_IS_FILE, geometry, GMT_IN, NULL, filelist[k])) == GMT_NOTSET) {
				GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Data: Could not register file for input: \n", filelist[k]);
				return_null (API, API->error);
			}
			if ((item = gmtapi_validate_id (API, family, in_ID, GMT_IN, GMTAPI_MODULE_INPUT)) == GMT_NOTSET)
				return_null (API, API->error);	/* Some internal error... */
			API->object[item]->selected = true;
		}
		gmtlib_free_list (API->GMT, filelist, n_files);	/* Free the file list */
		in_ID = GMT_NOTSET;
	}
	else if ((family == GMT_IS_GRID || family == GMT_IS_IMAGE) && (mode & GMT_DATA_ONLY)) {	/* Case 4: Already registered when we obtained header, find object ID */
		if ((in_ID = api_is_registered (API, family, geometry, GMT_IN, mode, input, data)) == GMT_NOTSET) {
			if (input) gmt_M_str_free (input);
			return_null (API, GMT_OBJECT_NOT_FOUND);	/* Could not find it */
		}
		if (!full_region (wesn)) {	/* Must update subset selection */
			int item;
			if ((item = gmtapi_validate_id (API, family, in_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET) {
				if (input) gmt_M_str_free (input);
				return_null (API, API->error);
			}
			gmt_M_memcpy (API->object[item]->wesn, wesn, 4, double);
		}
	}
	else if (input) {	/* Case 1: Load from a single input, given source. Register it first. */
		unsigned int first = 0;
		first = gmt_download_file_if_not_found (API->GMT, input);	/* Deal with downloadable GMT data sets first */
		/* Must handle special case when a list of colors are given instead of a CPT name.  We make a temp CPT from the colors */
		if (family == GMT_IS_PALETTE && !just_get_data) { /* CPTs must be handled differently since the master files live in share/cpt and filename is missing .cpt */
			int c_err = 0;
			char CPT_file[GMT_LEN256] = {""}, *file = strdup (&input[first]);
			if ((c_err = api_colors2cpt (API, &file, &mode)) < 0) { /* Maybe converted colors to new CPT */
				gmt_M_str_free (input);
				gmt_M_str_free (file);
				return_null (API, GMT_CPT_READ_ERROR);	/* Failed in the conversion */
			}
			else if (c_err == 0) {	/* Regular cpt (master or local), append .cpt and set path */
				size_t len = strlen (file);
				char *ext = (len > 4 && strstr (file, ".cpt")) ? "" : ".cpt";
				if (!(strstr (file, "+U") || strstr (file, "+u")))	/* Only append extension and supply path if not containing +u|U */
					gmt_getsharepath (API->GMT, "cpt", file, ext, CPT_file, R_OK);
				else	/* Use name as is */
					strncpy (CPT_file, file, GMT_LEN256-1);
			}
			else	/* Got color list, now a temp CPT instead */
				strncpy (CPT_file, file, GMT_LEN256-1);
			gmt_M_str_free (file);	/* Free temp CPT name */
			if ((in_ID = GMT_Register_IO (API, family, method, geometry, GMT_IN, wesn, CPT_file)) == GMT_NOTSET) {
				gmt_M_str_free (input);
				return_null (API, API->error);
			}
		}
		else if ((in_ID = GMT_Register_IO (API, family|module_input, method, geometry, GMT_IN, wesn, &input[first])) == GMT_NOTSET)
			return_null (API, API->error);
	}
	else if (input == NULL && geometry) {	/* Case 2: Load from stdin.  Register stdin first */
		if ((in_ID = GMT_Register_IO (API, family|module_input, GMT_IS_STREAM, geometry, GMT_IN, wesn, API->GMT->session.std[GMT_IN])) == GMT_NOTSET)
			return_null (API, API->error);	/* Failure to register std??? */
	}
	else {	/* Case 3: input == NULL && geometry == 0, so use all previously registered sources (unless already used). */
		if (!multiple_files_ok (family))
			return_null (API, GMT_ONLY_ONE_ALLOWED);	/* Virtual source only applies to data and text tables */
		API->shelf = family;	/* Save which one it is so we know in api_get_data */
		API->module_input = true;	/* Since we are passing NULL as file name we must loop over registered resources */
	}
	if (just_get_data) {
		if ((item = gmtapi_validate_id (API, GMT_NOTSET, in_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET) {
			gmt_M_str_free (input);
			return_null (API, API->error);
		}
		/* Try to catch a matrix or vector masquerading as dataset by examining the object's actual family  */
		if (family == (unsigned int)API->object[item]->actual_family) {	/* True to its word, otherwise we fall through and read the data */
#ifdef DEBUG
			api_set_object (API, API->object[item]);
#endif
			if (reset) API->object[item]->status = 0;	/* Reset  to unread */
			return (api_pass_object (API, API->object[item], family, wesn));
		}
	}

	/* OK, try to do the importing */
	if (in_ID != GMT_NOTSET) {	/* Make sure we select the item we just registered */
		if ((item = gmtapi_validate_id (API, GMT_NOTSET, in_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET) {
			gmt_M_str_free (input);
			return_null (API, API->error);
		}
		API->object[item]->selected = true;	/* Make sure the item we want is now selected */
	}
	if ((new_obj = api_get_data (API, in_ID, mode, data)) == NULL)
		return_null (API, API->error);
	if (reset) API->object[item]->status = 0;	/* Reset  to unread */
	gmt_M_str_free (input);	/* Done with this variable) */
	API->module_input = false;	/* Reset to normal */

#ifdef DEBUG
	api_list_objects (API, "GMT_Read_Data");
#endif

	return (new_obj);		/* Return pointer to the data container */
}

#ifdef FORTRAN_API
void *GMT_Read_Data_ (unsigned int *family, unsigned int *method, unsigned int *geometry, unsigned int *mode, double *wesn, char *input, void *data, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Read_Data (GMT_FORTRAN, *family, *method, *geometry, *mode, wesn, input, data));
}
#endif

/*! . */
void *GMT_Read_Group (void *V_API, unsigned int family, unsigned int method, unsigned int geometry, unsigned int mode, double wesn[], void *sources, unsigned int *n_items, void *data) {
	/* Function to read a group of data files directly into program memory givin an array of objects.
	 * data is pointer to an existing array of grid container when we read a grid in two steps, otherwise use NULL.
	 * *n_items = 0: sources is a character string with wildcard-specification for file names.
	 * *n_items > 0: sources is an array of *n_items character strings with filenames.
	 * If n_items == NULL then it means 0 but we do not return back the number of items.
	 * Note: For DATASET and TEXTSET you can also use wildcard expressions in GMT_Read_Data but there we combine then into one data|test-set.
	 * Return: Pointer to array of data container, or NULL if there were errors (passed back via API->error).
	 */
	unsigned int n_files, k;
	char **file = NULL, *pattern = NULL;
	struct GMTAPI_CTRL *API = NULL;
	void **object = NULL;
	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);

	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	if (data && !(family == GMT_IS_GRID || family == GMT_IS_IMAGE)) {
		GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Group: data pointer must be NULL except for GRID and IMAGE\n");
		return_null (API, GMT_PTR_NOT_NULL);
	}
	if (n_items && *n_items > 0) {	/* Gave list of files */
		n_files = *n_items;
		file = (char **)sources;
	}
	else {	/* Gave wildcard espression(s) */
		pattern = (void *)sources;
		if ((n_files = (unsigned int)gmtlib_glob_list (API->GMT, pattern, &file)) == 0) {
			GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Group: Expansion of \"%s\" gave no results\n", pattern);
			return_null (API, GMT_OBJECT_NOT_FOUND);
		}
	}
	/* Reuse data or allocate empty array of containers */
	object = (data == NULL) ? api_alloc_object_array (API, n_files, family) : data;
	for (k = 0; k < n_files; k++) {
		if ((object[k] = GMT_Read_Data (API, family, method, geometry, mode, wesn, file[k], object[k])) == NULL)
			GMT_Report (API, GMT_MSG_NORMAL, "GMT_Read_Group: Reading of %s failed, returning NULL\n", file[k]);
	}
	gmtlib_free_list (API->GMT, file, n_files);	/* Free the file list */
	if (n_items) *n_items = n_files;	/* Return how many items we allocated, if n_items is not NULL */
	return (object);	/* Return pointer to the data containers */
}

#ifdef FORTRAN_API
void *GMT_Read_Group_ (unsigned int *family, unsigned int *method, unsigned int *geometry, unsigned int *mode, double *wesn, void *sources, unsigned int *n_items, void *data) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Read_Group (GMT_FORTRAN, *family, *method, *geometry, *mode, wesn, sources, n_items, data));
}
#endif

/*! . */
void *GMT_Duplicate_Data (void *V_API, unsigned int family, unsigned int mode, void *data) {
	/* Create an duplicate container of the requested kind and optionally allocate space
	 * or duplicate content.
	 * The known families are GMT_IS_{DATASET,TEXTSET,GRID,PALETTE,IMAGE,POSTSCRIPT}.
 	 * Pass mode as one of GMT_DUPLICATE_{NONE|ALLOC|DATA} to just duplicate the
	 * container and header structures, allocate space of same dimensions as original,
	 * or allocate space and duplicate contents.  For GMT_IS_{DATA|TEXT}SET you may add
	 * the modifiers GMT_ALLOC_VERTICAL or GMT_ALLOC_HORIZONTAL. Also, for GMT_IS_DATASET
	 * you can manipulate the incoming data->dim to overwrite the number of items allocated.
	 * [By default we follow the dimensions of hte incoming data].
	 *
	 * Return: Pointer to new resource, or NULL if an error (set via API->error).
	 */

	int object_ID, item;
	unsigned int geometry = 0U, pmode = 0U;
	void *new_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	GMT = API->GMT;
	switch (family) {	/* dataset, cpt, text, grid , image, vector, matrix */
		case GMT_IS_GRID:	/* GMT grid, allocate header but not data array */
			new_obj = gmt_duplicate_grid (GMT, data, mode);
			geometry = GMT_IS_SURFACE;
			break;
		case GMT_IS_IMAGE:	/* GMT image, allocate header but not data array */
			new_obj = gmtlib_duplicate_image (GMT, data, mode);
			geometry = GMT_IS_SURFACE;
			break;
		case GMT_IS_DATASET:	/* GMT dataset, allocate the requested tables, segments, rows, and columns */
			pmode = (mode & (GMT_ALLOC_VERTICAL + GMT_ALLOC_HORIZONTAL));	/* Just isolate any special allocation modes */
			mode -= pmode;	/* Remove the hor/ver flags from the rest of mode */
			if (mode == GMT_DUPLICATE_DATA)
				new_obj = gmt_duplicate_dataset (GMT, data, pmode, &geometry);
			else if (mode == GMT_DUPLICATE_ALLOC) {	/* Allocate data set of same size, possibly modulated by Din->dim (of > 0) and pmode */
				struct GMT_DATASET *Din = data;	/* We know this is a GMT_DATASET pointer */
				new_obj = gmt_alloc_dataset (GMT, data, Din->dim[GMT_ROW], Din->dim[GMT_COL], pmode);
				geometry = Din->geometry;
				gmt_M_memset (Din->dim, 4, uint64_t);	/* Reset alloc dimensions */
			}
			else {	/* Just want a dataset structure */
				struct GMT_DATASET *Din = data;	/* We know this is a GMT_DATASET pointer */
				new_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_DATASET);
				geometry = Din->geometry;
			}
			break;
		case GMT_IS_TEXTSET:	/* GMT text dataset, allocate the requested tables, segments, and rows */
			pmode = (mode & (GMT_ALLOC_VERTICAL + GMT_ALLOC_HORIZONTAL));	/* Just isolate any special allocation modes */
			mode -= pmode;	/* Remove the hor/ver flags from the rest of mode */
			if (mode == GMT_DUPLICATE_DATA)
				new_obj = gmtlib_duplicate_textset (GMT, data, pmode);
			else if (mode == GMT_DUPLICATE_ALLOC)	/* Allocate text set of same size, possibly modulated by pmode */
				new_obj =  gmtlib_alloc_textset (GMT, data, pmode);
			else	/* Just want a dataset structure */
				new_obj = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSET);
			geometry = GMT_IS_NONE;
			break;
		case GMT_IS_PALETTE:	/* GMT CPT, allocate one with space for dim[0] color entries */
			new_obj = gmtlib_duplicate_palette (GMT, data, 0);
			geometry = GMT_IS_NONE;
			break;
		case GMT_IS_POSTSCRIPT:	/* GMT PS, allocate one with space for the original */
			new_obj = gmtlib_duplicate_ps (GMT, data, 0);
			geometry = GMT_IS_NONE;
			break;
		default:
			API->error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	if (API->error) return_null (API, API->error);

	/* Now register this dataset so it can be deleted by GMT_Destroy_Data */
	if ((object_ID = GMT_Register_IO (API, family, GMT_IS_REFERENCE, geometry, GMT_IN, NULL, new_obj)) == GMT_NOTSET)
		return_null (API, API->error);	/* Failure to register */
	if ((item = gmtapi_validate_id (API, family, object_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
		return_null (API, API->error);
	API->object[item]->geometry = geometry;	/* Ensure same geometry */
	API->object[item]->data = new_obj;		/* Retain pointer to the allocated data so we use garbage collection later */

	GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Successfully duplicated a %s\n", GMT_family[family]);
#ifdef DEBUG
	api_list_objects (API, "GMT_Duplicate_Data");
#endif

	return (new_obj);
}

#ifdef FORTRAN_API
void *GMT_Duplicate_Data_ (unsigned int *family,  unsigned int *mode, void *data) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Duplicate_Data (GMT_FORTRAN, *family, *mode, data));
}
#endif

/*! . */
int GMT_Write_Data (void *V_API, unsigned int family, unsigned int method, unsigned int geometry, unsigned int mode, double wesn[], const char *outfile, void *data) {
	/* Function to write data directly from program memory as a set (not record-by-record).
	 * We can combine the <register resource - export resource > sequence in
	 * one combined function.  See GMT_Register_IO for details on arguments.
	 * Here, *data is the pointer to the data object to save (CPT, dataset, textset, Grid)
	 * Case 1: outfile != NULL: Register this as the destination and export data.
	 * Case 2: outfile == NULL: Register stdout as the destination and export data.
	 * Case 3: geometry == 0: Use a previously registered single destination.
	 * While only one output destination is allowed, for DATA|TEXTSETS one can
	 * have the tables and even segments be written to individual files (see the mode
	 * description in the documentation for how to enable this feature.)
	 * Return: false if all is well, true if there was an error (and set API->error).
	 */
	unsigned int n_reg;
	int out_ID;
	char *output = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (data == NULL) return_error (V_API, GMT_PTR_IS_NULL);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if (outfile) output = strdup (outfile);

	if (output) {	/* Case 1: Save to a single specified destination (file or memory).  Register it first. */
		if ((out_ID = api_memory_registered (API, family, GMT_OUT, output)) != GMT_NOTSET) {
			/* Output is a memory resource, passed via a @GMTAPI@-###### file name, and ###### is the out_ID.
			   In this case we must make some further checks.  We need to find the API object that holds data.
			   We do this below and get in_ID (the id of the data to write), while out_ID is the id of where
			   things go (the output "memory").  Having the in_ID we get the array index in_item that matches
			   this ID and of the correct family.  We set direction to GMT_NOTSET since otherwise we may be
			   denied a hit as we don't really know what the direction is for in_ID.  Once in_item has been
			   secured we transfer ownership of this data from the in_ID object to the out_ID object.  That
			   way we avoid accidental premature freeing of the data object via the in_ID object since it now
			   will live on via out_ID and outlive the current module.
			    */
			int in_ID = GMT_NOTSET,  in_item = GMT_NOTSET;
			in_ID = api_get_object (API, family, data);	/* Get the object ID of the input source */
			if (in_ID != GMT_NOTSET) in_item = gmtapi_validate_id (API, family, in_ID, GMT_NOTSET, GMT_NOTSET);	/* Get the item in the API array; pass dir = GMT_NOTSET to bypass status check */
			if (in_item != GMT_NOTSET) {
				int out_item = gmtapi_validate_id (API, GMT_NOTSET, out_ID, GMT_OUT, GMT_NOTSET);	/* Get the item in the API array; pass family = GMT_NOTSET to bypass status check */
				GMT_Report (API, GMT_MSG_DEBUG, "GMT_Write_Data: Writing %s to memory object %d from object %d which transfers ownership\n", GMT_family[family], out_ID, in_ID);
				if (API->object[out_item]->method < GMT_IS_VECTOR) API->object[in_item]->no_longer_owner = true;	/* Since we have passed the content onto an output object */
				if (!API->object[out_item]->filename) API->object[out_item]->filename = strdup (output);
			}
		}	/* else it is a regular file and we just register it and get the new out_ID needed below */
		else if ((out_ID = GMT_Register_IO (API, family, method, geometry, GMT_OUT, wesn, output)) == GMT_NOTSET) return_error (API, API->error);
	}
	else if (output == NULL && geometry) {	/* Case 2: Save to stdout.  Register stdout first. */
		if (family == GMT_IS_GRID) return_error (API, GMT_STREAM_NOT_ALLOWED);	/* Cannot write grids to stream */
		if ((out_ID = GMT_Register_IO (API, family, GMT_IS_STREAM, geometry, GMT_OUT, wesn, API->GMT->session.std[GMT_OUT])) == GMT_NOTSET) return_error (API, API->error);	/* Failure to register std??? */
	}
	else {	/* Case 3: output == NULL && geometry == 0, so use the previously registered destination */
		if ((n_reg = gmtapi_count_objects (API, family, geometry, GMT_OUT, &out_ID)) != 1) return_error (API, GMT_NO_OUTPUT);	/* There is no registered output */
	}
	/* With out_ID in hand we can now put the data where it should go */
	if (api_put_data (API, out_ID, mode, data) != GMT_OK) return_error (API, API->error);
	gmt_M_str_free (output);	/* Done with this variable */

#ifdef DEBUG
	api_list_objects (API, "GMT_Write_Data");
#endif
	return (GMT_OK);	/* No error encountered */
}

#ifdef FORTRAN_API
int GMT_Write_Data_ (unsigned int *family, unsigned int *method, unsigned int *geometry, unsigned int *mode, double *wesn, char *output, void *data, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Write_Data (GMT_FORTRAN, *family, *method, *geometry, *mode, wesn, output, data));
}
#endif

static inline int api_wind_to_next_datarecord (int64_t *count, struct GMT_DATASET *D, unsigned int mode) {
	/* Increment row, seg, tbl to next record and return current record status */
	if (count[GMT_SEG] == -1) {	/* Special flag to processes table header(s) */
		if (count[GMTAPI_HDR_POS] < D->table[count[GMT_TBL]]->n_headers) {	/* Must first handle table headers */
			count[GMTAPI_HDR_POS]++;	/* Increment counter for each one we return until done */
			return GMT_IO_TABLE_HEADER;
		}
		/* Must be out of table headers - time for the segment header */
		count[GMT_SEG] = count[GMT_ROW] = 0;
		return GMT_IO_SEGMENT_HEADER;
	}
	if (count[GMT_ROW] == (int64_t)D->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->n_rows) {	/* Previous record was last in segment, go to next */
		count[GMT_SEG]++;	/* Next segment number */
		count[GMT_ROW] = 0;
		if (count[GMT_SEG] == (int64_t)D->table[count[GMT_TBL]]->n_segments) {		/* Also the end of a table ("file") */
			count[GMT_TBL]++;	/* Next table number */
			count[GMT_SEG] = -1;	/* Reset to start at first segment in this table */
			count[GMTAPI_HDR_POS] = 0;	/* Ready to process headers from next table */
			if (count[GMT_TBL] == (int64_t)D->n_tables)	/* End of entire data set */
				return GMT_IO_EOF;
			/* Just end of a file */
			if (mode & GMT_READ_FILEBREAK)	/* Return empty handed to indicate a break between files */
				return GMT_IO_NEXT_FILE;
		}
		return GMT_IO_SEGMENT_HEADER;
	}
	/* No drama, here we have a data record just go to next row */
	return GMT_IO_DATA_RECORD;
}

static inline int api_wind_to_next_textrecord (int64_t *count, struct GMT_TEXTSET *D, unsigned int mode) {
	/* Increment row, seg, tbl to next record and return current record status */
	if (count[GMT_SEG] == -1) {	/* Special flag to processes table header(s) */
		if (count[GMTAPI_HDR_POS] < D->table[count[GMT_TBL]]->n_headers) {	/* Must first handle table headers */
			count[GMTAPI_HDR_POS]++;	/* Increment counter for each one we return until done */
			return GMT_IO_TABLE_HEADER;
		}
		/* Must be out of table headers - time for the segment header */
		count[GMT_SEG] = count[GMT_ROW] = 0;
		return GMT_IO_SEGMENT_HEADER;
	}
	if (count[GMT_ROW] == (int64_t)D->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->n_rows) {	/* Previous record was last in segment, go to next */
		count[GMT_SEG]++;	/* Next segment number */
		count[GMT_ROW] = 0;
		if (count[GMT_SEG] == (int64_t)D->table[count[GMT_TBL]]->n_segments) {		/* Also the end of a table ("file") */
			count[GMT_TBL]++;	/* Next table number */
			count[GMT_SEG] = -1;	/* Reset to start at first segment in this table */
			count[GMTAPI_HDR_POS] = 0;	/* Ready to process headers from next table */
			if (count[GMT_TBL] == (int64_t)D->n_tables)	/* End of entire data set */
				return GMT_IO_EOF;
			/* Just end of a file */
			if (mode & GMT_READ_FILEBREAK)	/* Return empty handed to indicate a break between files */
				return GMT_IO_NEXT_FILE;
		}
		return GMT_IO_SEGMENT_HEADER;
	}
	/* No drama, here we have a data record just go to next row */
	return GMT_IO_DATA_RECORD;
}

/*! . */
int GMT_Set_Geometry (void *V_API, unsigned int direction, unsigned int geometry) {
	/* Sets the geometry of direction resource for record-by-record i/o.
	 * This currently only applies to external interfaces receiving data via rec-by-rc writing.
	 */
	unsigned int method;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	if (!API->io_enabled[GMT_OUT]) return_error (API, GMT_ACCESS_NOT_ENABLED);
	API->error = GMT_NOERROR;

	S_obj = API->object[API->current_item[direction]];	/* Shorthand for the data source we are working on */
	if (S_obj == NULL) return_error (API, GMT_OBJECT_NOT_FOUND);	/* No such object */
	method = api_set_method (S_obj);	/* Get the actual method to use */
	switch (method) {	/* File, array, stream etc ? */
		case GMT_IS_DUPLICATE:
		case GMT_IS_REFERENCE:
			if (S_obj->family == GMT_IS_DATASET) {
				struct GMT_DATASET *D_obj = S_obj->resource;
				if (!D_obj)	/* Not allocated yet?*/
					GMT_Report (API, GMT_MSG_DEBUG, "GMTAPI: Warning: GMT_Set_Geometry called but no object available\n");
				else
					D_obj->geometry = geometry;
			}
			break;
		default:	/* For all others there is no geometry requirement, so quietly skip */
			break;
	}
	return GMT_NOERROR;
}

#ifdef FORTRAN_API
int GMT_Set_Geometry_ (unsigned int *direction, unsigned int *geometry) {	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Set_Geometry (GMT_FORTRAN, *direction, *geometry));
}
#endif

/*! . */
void *GMT_Get_Record (void *V_API, unsigned int mode, int *retval) {
	/* Retrieves the next data record from the virtual input source and
	 * returns the number of columns found via *retval (unless retval == NULL).
	 * If current record is a segment header then we return 0.
	 * If we reach EOF then we return EOF.
	 * mode is either GMT_READ_DATA (data columns), GMT_READ_TEXT (text string) or
	 *	GMT_READ_MIXED (expect data but tolerate read errors).
	 * Also, if (mode | GMT_READ_FILEBREAK) is true then we will return empty-handed
	 *	when we get to the end of a file except the final file (which is EOF).
	 *	The calling module can then take actions appropriate between data files.
	 * The double array OR text string is returned via the pointer *record.
	 * If not a data record we return NULL, and pass status via API->GMT->current.io.status.
	 */

	int status;
	unsigned int method;
	int64_t *count = NULL, n_fields = 0;
	uint64_t col, ij, n_use, col_pos, n_columns = 0;
	bool get_next_record;
	void *record = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_getfunction api_get_val = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_TEXTSET *DT_obj = NULL;
	struct GMT_DATASET *DS_obj = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if (retval) *retval = 0;
	if (!API->io_enabled[GMT_IN]) return_null (API, GMT_ACCESS_NOT_ENABLED);

	S_obj = API->object[API->current_item[GMT_IN]];	/* Shorthand for the current data source we are working on */
	API->GMT->current.io.read_mixed = (mode == GMT_READ_MIXED);	/* Cannot worry about constant # of columns if text is present */
	GMT = API->GMT;

	method = api_set_method (S_obj);	/* Get the actual method to use */
	do {	/* We do this until we can secure the next record or we run out of records (and return EOF) */
		get_next_record = false;	/* We expect to read one data record and return */
		GMT->current.io.status = 0;	/* Initialize status to OK */
		if (S_obj->status == GMT_IS_USED) {		/* Finished reading from this resource, go to next resource */
			if (GMT->current.io.ogr == GMT_OGR_TRUE) return_null (API, GMT_OGR_ONE_TABLE_ONLY);	/* Only allow single tables if GMT/OGR */
			if ((status = api_next_data_object (API, S_obj->family, GMT_IN)) == EOF)	/* That was the last source, return */
				n_fields = EOF;
			else if (status) {	/* This is not good... */
				return_null (API, GMT_NO_RESOURCES);
			}
			else {
				S_obj = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
				get_next_record = true;				/* Since we haven't read the next record yet */
			}
			continue;
		}
		switch (method) {
			case GMT_IS_FILE:	/* File, stream, and fd are all the same for us, regardless of data or text input */
		 	case GMT_IS_STREAM:
		 	case GMT_IS_FDESC:
		 		record = S_obj->import (GMT, S_obj->fp, &(S_obj->n_expected_fields), &status);	/* Get that next record */
				n_fields = S_obj->n_columns = status;	/* Get that next record */
				if (GMT->current.io.status & GMT_IO_EOF) {			/* End-of-file in current file (but there may be many files) */
					S_obj->status = GMT_IS_USED;	/* Mark as read */
					if (S_obj->close_file) {	/* Close if it was a file that we opened earlier */
						gmt_fclose (GMT, S_obj->fp);
						S_obj->close_file = false;
					}
					if (api_next_data_object (API, S_obj->family, GMT_IN) == EOF)	/* That was the last source, return */
						n_fields = EOF;					/* EOF is ONLY returned when we reach the end of the LAST data file */
					else if (mode & GMT_READ_FILEBREAK) {			/* Return empty handed to indicate a break between files */
						n_fields = GMT_IO_NEXT_FILE;			/* We flag this situation with a special return value */
						GMT->current.io.status = GMT_IO_NEXT_FILE;
					}
					else {	/* Get ready to read the next data file */
						S_obj = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
						get_next_record = true;				/* Since we haven't read the next record yet */
					}
					GMT->current.io.tbl_no++;				/* Update number of tables we have processed */
				}
				else
					S_obj->status = GMT_IS_USING;				/* Mark as being read */

				if (gmt_M_rec_is_data (GMT) && S_obj->n_expected_fields != GMT_MAX_COLUMNS)
					GMT->common.b.ncol[GMT_IN] = S_obj->n_expected_fields;	/* Set the actual column count */
				break;

			case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* Here we copy/read from a user memory location */
			case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
				if (S_obj->rec >= S_obj->n_rows) {	/* Our only way of knowing we are done is to quit when we reach the number of rows that was registered */
					S_obj->status = GMT_IS_USED;	/* Mark as read */
					if (api_next_data_object (API, S_obj->family, GMT_IN) == EOF) {	/* That was the last source, return */
						n_fields = EOF;					/* EOF is ONLY returned when we reach the end of the LAST data file */
						GMT->current.io.status = GMT_IO_EOF;
					}
					else if (mode & GMT_READ_FILEBREAK) {			/* Return empty handed to indicate a break between files */
						n_fields = GMT_IO_NEXT_FILE;			/* We flag this situation with a special return value */
						GMT->current.io.status = GMT_IO_NEXT_FILE;
					}
					else {	/* Get ready to read the next data file */
						S_obj = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
						get_next_record = true;				/* Since we haven't read the next record yet */
					}
				}
				else {	/* Read from this resource */
					S_obj->status = GMT_IS_USING;				/* Mark as being read */
					M_obj = S_obj->resource;
					n_use = api_n_cols_needed_for_gaps (GMT, S_obj->n_columns);
					api_update_prev_rec (GMT, n_use);
					n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : S_obj->n_columns;
					GMT_2D_to_index = api_get_2d_to_index (API, M_obj->shape, GMT_GRID_IS_REAL);
					api_get_val = api_select_get_function (API, M_obj->type);
					for (col = 0; col < n_columns; col++) {	/* We know the number of columns from registration */
						col_pos = api_pick_in_col_number (GMT, (unsigned int)col);
						ij = GMT_2D_to_index (S_obj->rec, col_pos, M_obj->dim);
						api_get_val (&(M_obj->data), ij, &(GMT->current.io.curr_rec[col]));
					}
					S_obj->rec++;
					if ((status = api_bin_input_memory (GMT, S_obj->n_columns, n_use)) < 0) {	/* Process the data record */
						if (status == GMTAPI_GOT_SEGGAP)	 /* Since we inserted a segment header we must revisit this record as first in next segment */
							S_obj->rec--, API->current_rec[GMT_IN]--;
						record = NULL;
					}
					else	/* Valid data record */
						record = GMT->current.io.curr_rec;
					n_fields = n_columns;
				}
				break;

			 case GMT_IS_DUPLICATE|GMT_VIA_VECTOR:	/* Here we copy from a user memory location that points to an array of column vectors */
			 case GMT_IS_REFERENCE|GMT_VIA_VECTOR:
				if (S_obj->rec == S_obj->n_rows) {	/* Our only way of knowing we are done is to quit when we reach the number of rows in this vector */
					S_obj->status = GMT_IS_USED;	/* Mark as read */
					if (api_next_data_object (API, S_obj->family, GMT_IN) == EOF) {	/* That was the last source, return */
						n_fields = EOF;					/* EOF is ONLY returned when we reach the end of the LAST data file */
						GMT->current.io.status = GMT_IO_EOF;
					}
					else if (mode & GMT_READ_FILEBREAK) {			/* Return empty handed to indicate a break between files */
						n_fields = GMT_IO_NEXT_FILE;			/* We flag this situation with a special return value */
						GMT->current.io.status = GMT_IO_NEXT_FILE;
					}
					else {	/* Get ready to read the next data file */
						S_obj = API->object[API->current_item[GMT_IN]];	/* Shorthand for the next data source to work on */
						get_next_record = true;				/* Since we haven't read the next record yet */
					}
				}
				else {	/* Read from this resource */
					S_obj->status = GMT_IS_USING;				/* Mark as being read */
					V_obj = S_obj->resource;
					n_use = api_n_cols_needed_for_gaps (GMT, S_obj->n_columns);
					api_update_prev_rec (GMT, n_use);
					api_get_val = api_select_get_function (API, V_obj->type[0]);	/* For 1st column and probably all of them - if not we reset below */
					n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : S_obj->n_columns;
					for (col = 0; col < n_columns; col++) {	/* We know the number of columns from registration */
						col_pos = api_pick_in_col_number (GMT, (unsigned int)col);
						if (col_pos && V_obj->type[col_pos] != V_obj->type[col_pos-1]) api_get_val = api_select_get_function (API, V_obj->type[col_pos]);
						api_get_val (&(V_obj->data[col_pos]), S_obj->rec, &(GMT->current.io.curr_rec[col]));
					}
					S_obj->rec++;
					if ((status = api_bin_input_memory (GMT, S_obj->n_columns, n_use)) < 0) {	/* Process the data record */
						if (status == GMTAPI_GOT_SEGGAP)	 /* Since we inserted a segment header we must revisit this record as first in next segment */
							S_obj->rec--, API->current_rec[GMT_IN]--;
						record = NULL;
					}
					else	/* Valid data record */
						record = GMT->current.io.curr_rec;
					n_fields = n_columns;
				}
				break;

			case GMT_IS_REFERENCE:	/* Only for textsets and datasets */
				count = GMT->current.io.curr_pos[GMT_IN];	/* Shorthand used below */
				if (S_obj->family == GMT_IS_DATASET) {
					DS_obj = S_obj->resource;	/* Get the right dataset */
					status = api_wind_to_next_datarecord (count, DS_obj, mode);	/* Get current record status and wind counters if needed */
					n_columns = (GMT->common.i.active) ? GMT->common.i.n_cols : DS_obj->n_columns;
					switch (status) {
						case GMT_IO_DATA_RECORD:	/* Got a data record */
							S_obj->status = GMT_IS_USING;		/* Mark this resource as currently being read */
							for (col = 0; col < n_columns; col++) {	/* Copy from row to curr_rec */
								col_pos = api_pick_in_col_number (GMT, (unsigned int)col);
								GMT->current.io.curr_rec[col] = DS_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->data[col_pos][count[GMT_ROW]];
							}
							record = GMT->current.io.curr_rec;	/* We will return this double array */
							n_fields = GMT->common.b.ncol[GMT_IN] = n_columns;
							count[GMT_ROW]++;	/* Advance to next row for next time GMT_Get_Record is called */
							break;
						case GMT_IO_SEGMENT_HEADER:	/* Segment break */
							if (DS_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->header)
								strncpy (GMT->current.io.segment_header, DS_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->header, GMT_BUFSIZ-1);
							else
								GMT->current.io.segment_header[0] = '\0';	/* No header for this segment */
							record = NULL;	/* No data record to return */
							n_fields = 0;
							break;
						case GMT_IO_TABLE_HEADER:	/* Table header(s) */
							strncpy (GMT->current.io.record, DS_obj->table[count[GMT_TBL]]->header[count[GMTAPI_HDR_POS]-1], GMT_BUFSIZ-1);
							record = NULL;	/* No data record to return */
							n_fields = 0;
							break;
						case GMT_IO_NEXT_FILE:	/* End of a table but more tables to come */
							record = NULL;	/* No data record to return */
							n_fields = GMT_IO_NEXT_FILE;
							break;
						case GMT_IO_EOF:	/* End of entire data set */
							S_obj->status = GMT_IS_USED;	/* Mark this dataset as finished */
							record = NULL;	/* No more to return anyway */
							n_fields = EOF;
							break;
					}
					GMT->current.io.status = status;
				}
				else if (S_obj->family == GMT_IS_TEXTSET) {
					DT_obj = S_obj->resource;
					status = api_wind_to_next_textrecord (count, DT_obj, mode);	/* Get current record status and wind counters if needed */
					switch (status) {
						case GMT_IO_DATA_RECORD:	/* Got a data record */
							S_obj->status = GMT_IS_USING;		/* Mark this resource as currently being read */
							record = strncpy (GMT->current.io.record, DT_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->data[count[GMT_ROW]], GMT_BUFSIZ-1);	/* Copy record */
							if (GMT->current.io.record[0] == GMT->current.setting.io_seg_marker[GMT_IN]) {	/* Got a seg header pretending to be data */
								strncpy (GMT->current.io.segment_header, gmtlib_trim_segheader (GMT, GMT->current.io.record), GMT_BUFSIZ-1);
								record = NULL;	/* No data record to return */
								n_fields = 0;
								status = GMT_IO_SEGMENT_HEADER;	/* Change our mind */
							}
							else {	/* A read data record */
								n_fields = 1;	/* Can only be one string since it is the entire record */
							}
							count[GMT_ROW]++;	/* Advance to next row for next time GMT_Get_Record is called */
							break;
						case GMT_IO_SEGMENT_HEADER:	/* Segment break */
							if (DT_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->header)	/* Copy segment header */
								strncpy (GMT->current.io.segment_header, DT_obj->table[count[GMT_TBL]]->segment[count[GMT_SEG]]->header, GMT_BUFSIZ-1);
							else
								GMT->current.io.segment_header[0] = '\0';	/* No header for this segment */
							record = NULL;	/* No data record to return */
							n_fields = 0;
							break;
						case GMT_IO_TABLE_HEADER:	/* Table header(s) */
							strncpy (GMT->current.io.record, DT_obj->table[count[GMT_TBL]]->header[count[GMTAPI_HDR_POS]-1], GMT_BUFSIZ-1);
							record = NULL;	/* No data record to return */
							n_fields = 0;
							break;
						case GMT_IO_NEXT_FILE:		/* End of a table but more tables to come */
							record = NULL;	/* No data record to return */
							n_fields = GMT_IO_NEXT_FILE;
							break;
						case GMT_IO_EOF:	/* End of entire data set */
							S_obj->status = GMT_IS_USED;	/* Mark this dataset as finished */
							record = NULL;	/* No more to return anyway */
							n_fields = EOF;
							break;
					}
					GMT->current.io.status = status;
				}
				break;
			default:
				GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: GMT_Get_Record called with illegal method\n");
				break;
		}
	} while (get_next_record);

	if (!(n_fields == EOF || n_fields == GMT_IO_NEXT_FILE)) API->current_rec[GMT_IN]++;	/* Increase record count, unless EOF */

	if (retval) *retval = (int)n_fields;	/* Requested we return the number of fields found */
	return (record);	/* Return pointer to current record */
}

#ifdef FORTRAN_API
void *GMT_Get_Record_ (unsigned int *mode, int *status) {	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Record (GMT_FORTRAN, *mode, status));
}
#endif

/*! . */
int GMT_Put_Record (void *V_API, unsigned int mode, void *record) {
	/* Writes a single data record to destimation.
	 * We use mode to signal the kind of record:
	 *   GMT_WRITE_TABLE_HEADER: Write an ASCII table header
	 *   GMT_WRITE_SEGMENT_HEADER: Write an ASCII or binary segment header
	 *   GMT_WRITE_DATA:    Write an ASCII or binary data record
	 *   GMT_WRITE_TEXT:      Write an ASCII data record
	 * For text: If record == NULL use internal current record or header.
	 * Returns 0 if a record was written successfully (See what -s[r] can do).
	 * If an error occurs we return GMT_NOTSET and set API->error.
	 */
	int error = 0;
	unsigned int method;
	int64_t *count = NULL;
	uint64_t col, ij;
	char *s = NULL;
	double *d = NULL, value;
	p_func_uint64_t GMT_2D_to_index = NULL;
	GMT_putfunction api_put_val = NULL;
	struct GMTAPI_DATA_OBJECT *S_obj = NULL;
	struct GMT_MATRIX *M_obj = NULL;
	struct GMT_VECTOR *V_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	if (!API->io_enabled[GMT_OUT]) return_error (API, GMT_ACCESS_NOT_ENABLED);
	API->error = GMT_NOERROR;

	S_obj = API->object[API->current_item[GMT_OUT]];	/* Shorthand for the data source we are working on */
	if (S_obj->status == GMT_IS_USED) return_error (API, GMT_WRITTEN_ONCE);	/* Only allow writing of a data set once [unless we reset status] */
	method = api_set_method (S_obj);	/* Get the actual method to use */
	switch (method) {	/* File, array, stream etc ? */
		case GMT_IS_FILE:
	 	case GMT_IS_STREAM:
	 	case GMT_IS_FDESC:
			switch (mode) {
				case GMT_WRITE_TABLE_HEADER:	/* Export a table header record; skip if binary */
					s = (record) ? record : API->GMT->current.io.record;	/* Default to last input record if NULL */
					gmtlib_write_tableheader (API->GMT, S_obj->fp, s);	error = 1;	/* Write one item */
					break;
				case GMT_WRITE_SEGMENT_HEADER:	/* Export a segment header record; write NaNs if binary  */
					if (record) strncpy (API->GMT->current.io.segment_header, record, GMT_BUFSIZ-1);	/* Default to last segment record if NULL */
					gmt_write_segmentheader (API->GMT, S_obj->fp, API->GMT->common.b.ncol[GMT_OUT]);	error = 1;	/* Write one item */
					break;
				case GMT_WRITE_DATA:		/* Export either a formatted ASCII data record or a binary record */
					if (API->GMT->common.b.ncol[GMT_OUT] == UINT_MAX) API->GMT->common.b.ncol[GMT_OUT] = API->GMT->common.b.ncol[GMT_IN];
					error = API->GMT->current.io.output (API->GMT, S_obj->fp, API->GMT->common.b.ncol[GMT_OUT], record);
					break;
				case GMT_WRITE_TEXT:		/* Export the current text record; skip if binary */
					s = (record) ? record : API->GMT->current.io.record;
					gmtlib_write_textrecord (API->GMT, S_obj->fp, s);	error = 1;	/* Write one item */
					break;
				case GMT_WRITE_TABLE_START:	/* Write title and command to start of file; skip if binary */
					gmtlib_write_newheaders (API->GMT, S_obj->fp, S_obj->n_columns);	error = 1;	/* Write one item */
					break;
				default:
					GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: GMT_Put_Record called with illegal mode %u\n", mode);
					return_error (API, GMT_NOT_A_VALID_IO_MODE);
					break;
			}
			break;

		case GMT_IS_DUPLICATE:	/* Fill in a DATASET structure with one table only */
		case GMT_IS_REFERENCE:	/* Fill in a DATASET structure with one table only */
			if (S_obj->family == GMT_IS_DATASET) {
				struct GMT_DATASET *D_obj = S_obj->resource;
				struct GMT_DATATABLE *T_obj = NULL;
				if (!D_obj) {	/* First time allocation of the single output table */
					D_obj = gmtlib_create_dataset (API->GMT, 1, GMT_TINY_CHUNK, 0, 0, S_obj->geometry, true);	/* 1 table, segments array; no cols or rows yet */
					S_obj->resource = D_obj;	/* Save this pointer for next time we call GMT_Put_Record */
					API->GMT->current.io.curr_pos[GMT_OUT][GMT_SEG] = -1;	/* Start at seg = -1 and increment at first segment header */
					if (API->GMT->common.b.ncol[GMT_OUT] == 0 && API->GMT->common.b.ncol[GMT_IN] < GMT_MAX_COLUMNS)
						API->GMT->common.b.ncol[GMT_OUT] = API->GMT->common.b.ncol[GMT_IN];
					D_obj->n_columns = D_obj->table[0]->n_columns = API->GMT->common.b.ncol[GMT_OUT];
				}
				T_obj = D_obj->table[0];	/* GMT_Put_Record only writes one table with one or more segments */
				if ((D_obj->n_columns == 0 || D_obj->n_columns == GMT_MAX_COLUMNS || D_obj->n_columns < API->GMT->common.b.ncol[GMT_OUT]) && mode == GMT_WRITE_DATA) {	/* Number of columns not set or set incorrectly, see if -b has it */
					if (API->GMT->common.b.ncol[GMT_OUT])
						D_obj->n_columns = T_obj->n_columns = API->GMT->common.b.ncol[GMT_OUT];
					else {
						GMT_Report (API, GMT_MSG_DEBUG, "GMTAPI: Error: GMT_Put_Record does not know the number of columns\n");
						return_error (API, GMT_N_COLS_NOT_SET);
					}
				}
				count = API->GMT->current.io.curr_pos[GMT_OUT];	/* Short hand to counters for table (not used as == 0), segment, row */
				switch (mode) {
					case GMT_WRITE_TABLE_HEADER:	/* Export a table header record; skip if binary */
						s = (record) ? record : API->GMT->current.io.record;	/* Default to last input record if NULL */
						/* Hook into table header list */
						if (count[GMT_SEG] == -1) {	/* Only allow headers for first segment in a table */
							T_obj->header = gmt_M_memory (API->GMT, T_obj->header, T_obj->n_headers+1, char *);
							T_obj->header[T_obj->n_headers++] = strdup (s);
						}
						break;
					case GMT_WRITE_SEGMENT_HEADER:	/* Export a segment header record; write NaNs if binary  */
						count[GMT_SEG]++;	/* Start of new segment */
						if (count[GMT_SEG]) {	/* Must first copy over records for the previous segments; last (or only) segment will be done by GMT_End_IO */
							gmtlib_assign_segment (API->GMT, T_obj->segment[count[GMT_SEG]-1], count[GMT_ROW], T_obj->n_columns);	/* Allocate and place arrays into previous segment */
							count[GMT_ROW] = 0;	/* Reset for next segment */
							T_obj->n_segments++;
						}
						if (count[GMT_SEG] == (int64_t)T_obj->n_alloc) {	/* Extend but set new members to NULL */
							size_t was = T_obj->n_alloc;
							T_obj->segment = gmt_M_malloc (API->GMT, T_obj->segment, count[GMT_SEG], &T_obj->n_alloc, struct GMT_DATASEGMENT *);
							gmt_M_memset (&T_obj->segment[was], T_obj->n_alloc - was, struct GMT_DATASEGMENT *);
						}
						if (!T_obj->segment[count[GMT_SEG]]) T_obj->segment[count[GMT_SEG]] = gmt_M_memory (API->GMT, NULL, 1, struct GMT_DATASEGMENT);
						s = (record) ? record : API->GMT->current.io.segment_header;	/* Default to last segment header record if NULL */
						if (s) {	/* Found a segment header */
							if (T_obj->segment[count[GMT_SEG]]->header) gmt_M_str_free (T_obj->segment[count[GMT_SEG]]->header);	/* Hm, better free the old guy before strdup'ing a new one */
							T_obj->segment[count[GMT_SEG]]->header = strdup (s);
						}
						break;
					case GMT_WRITE_DATA:		/* Export a segment row */
						if (gmt_skip_output (API->GMT, record, T_obj->n_columns))	/* Record was skipped via -s[a|r] */
							break;
						if (count[GMT_SEG] == -1) {	/* Most likely a file with one segment but no segment header */
							GMT_Report (API, GMT_MSG_DEBUG, "GMTAPI: Internal Warning: GMT_Put_Record (double) called before any segments declared\n");
							count[GMT_SEG] = 0;
						}
						gmt_prep_tmp_arrays (API->GMT, count[GMT_ROW], T_obj->n_columns);	/* Init or reallocate tmp read vectors */
						for (col = 0; col < T_obj->n_columns; col++) {
							API->GMT->hidden.mem_coord[col][count[GMT_ROW]] = ((double *)record)[col];
							if (API->GMT->current.io.col_type[GMT_OUT][col] & GMT_IS_LON) gmt_lon_range_adjust (API->GMT->current.io.geo.range, &(API->GMT->hidden.mem_coord[col][count[GMT_ROW]]));
						}
						count[GMT_ROW]++;	/* Increment rows in this segment */
						break;
					case GMT_WRITE_TABLE_START:	/* Write title and command to start of file; skip if binary */
						break;	/* Ignore for this method */
					default:
						GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: GMT_Put_Record called with illegal mode %u\n", mode);
						return_error (API, GMT_NOT_A_VALID_IO_MODE);
						break;
				}
			}
			else {	/* TEXTSET */
				struct GMT_TEXTSET *D_obj = S_obj->resource;
				struct GMT_TEXTTABLE *T_obj = NULL;
				if (!D_obj) {	/* First time allocation of one table */
					D_obj = gmtlib_create_textset (API->GMT, 1, GMT_TINY_CHUNK, 0, true);
					S_obj->resource = D_obj;
					API->GMT->current.io.curr_pos[GMT_OUT][GMT_SEG] = -1;	/* Start at seg = -1 and increment at first segment header */
				}
				T_obj = D_obj->table[0];	/* GMT_Put_Record only writes one table */
				count = API->GMT->current.io.curr_pos[GMT_OUT];	/* Short hand to counters for table, segment, row */
				switch (mode) {
					case GMT_WRITE_TABLE_HEADER:	/* Export a table header record; skip if binary */
						s = (record) ? record : API->GMT->current.io.record;	/* Default to last input record if NULL */
						/* Hook into table header list */
						if (count[GMT_SEG] == -1) {	/* Only allow headers for first segment in a table */
							T_obj->header = gmt_M_memory (API->GMT, T_obj->header, T_obj->n_headers+1, char *);
							T_obj->header[T_obj->n_headers++] = strdup (s);
						}
						break;
					case GMT_WRITE_SEGMENT_HEADER:	/* Export a segment header record; write NaNs if binary  */
						count[GMT_SEG]++;	/* Start of new segment */
						if (count[GMT_SEG] > 0) {
							T_obj->segment[count[GMT_SEG]-1]->data = gmt_M_memory (API->GMT, T_obj->segment[count[GMT_SEG]-1]->data, count[GMT_ROW], char *);
							T_obj->segment[count[GMT_SEG]-1]->n_rows = count[GMT_ROW];
							T_obj->n_segments++;
							count[GMT_ROW] = 0;	/* Reset for next segment */
						}
						if (count[GMT_SEG] == (int64_t)T_obj->n_alloc) {	/* Allocate more but set pointers to NULL */
							size_t was = T_obj->n_alloc;
							T_obj->segment = gmt_M_malloc (API->GMT, T_obj->segment, count[GMT_SEG], &T_obj->n_alloc, struct GMT_TEXTSEGMENT *);
							gmt_M_memset (&T_obj->segment[was], T_obj->n_alloc - was, struct GMT_TEXTSEGMENT *);
						}
						if (!T_obj->segment[count[GMT_SEG]]) T_obj->segment[count[GMT_SEG]] = gmt_M_memory (API->GMT, NULL, 1, struct GMT_TEXTSEGMENT);
						s = (record) ? record : API->GMT->current.io.segment_header;	/* Default to last segment header record if NULL */
						if (s) {	/* Found a segment header */
							if (T_obj->segment[count[GMT_SEG]]->header)
								gmt_M_str_free (T_obj->segment[count[GMT_SEG]]->header);	/* Hm, better free the old guy before strdup'ing a new one */
							T_obj->segment[count[GMT_SEG]]->header = strdup (s);
						}
						break;
					case GMT_WRITE_TEXT:		/* Export a record */
						if (count[GMT_SEG] == -1) {	/* Most likely a file with one segment but no segment header */
							GMT_Report (API, GMT_MSG_DEBUG, "GMTAPI: Internal Warning: GMT_Put_Record (text) called before any segments declared\n");
							count[GMT_SEG] = 0;
						}
						if (!T_obj->segment[count[GMT_SEG]]) T_obj->segment[count[GMT_SEG]] = gmt_M_memory (API->GMT, NULL, 1, struct GMT_TEXTSEGMENT);	/* Allocate new segment */
						if (count[GMT_ROW] == (int64_t)T_obj->segment[count[GMT_SEG]]->n_alloc) {	/* Allocate more records */
							T_obj->segment[count[GMT_SEG]]->n_alloc = (T_obj->segment[count[GMT_SEG]]->n_alloc == 0) ? GMT_CHUNK : T_obj->segment[count[GMT_SEG]]->n_alloc << 1;
							T_obj->segment[count[GMT_SEG]]->data = gmt_M_memory (API->GMT, NULL, T_obj->segment[count[GMT_SEG]]->n_alloc, char *);
						}
						T_obj->segment[count[GMT_SEG]]->data[count[GMT_ROW]] = strdup (record);
						count[GMT_ROW]++;
						break;
					case GMT_WRITE_TABLE_START:	/* Write title and command to start of file; skip if binary */
						break;	/* Ignore for this method */
					default:
						GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: GMT_Put_Record (text) called with illegal mode\n");
						return_error (API, GMT_NOT_A_VALID_IO_MODE);
						break;
				}
			}
			break;

		case GMT_IS_DUPLICATE|GMT_VIA_MATRIX:	/* Data matrix only */
		case GMT_IS_REFERENCE|GMT_VIA_MATRIX:
			/* At the first output record the output matrix has not been allocated.
			 * So first we do that, then later we can increment its size when needed.
			 * The realloc to final size takes place in GMT_End_IO. */
			if (S_obj->n_rows && S_obj->rec >= S_obj->n_rows)
				GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: GMT_Put_Record exceeding limits on rows(?) - possible bug\n");
			if (S_obj->resource == NULL) {	/* First time allocating space; S_obj->n_rows == S_obj->n_alloc == 0 */
				size_t size;
				col = (API->GMT->common.o.active) ? API->GMT->common.o.n_cols : API->GMT->common.b.ncol[GMT_OUT];	/* Number of columns needed to hold the data records */
				if (col == 0 && mode == GMT_WRITE_SEGMENT_HEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {
					/* Cannot place the NaN records since we don't know the number of columns yet */
					S_obj->delay++;
					S_obj->rec++;					/* Since the NaN-record is an actual data record that encodes a segment break */
					API->current_rec[GMT_OUT]++;	/* Since the NaN-record is an actual data record that encodes a segment break */
					break;
				}
				size = S_obj->n_alloc = GMT_CHUNK;
				M_obj = gmtlib_create_matrix (API->GMT, 1U, GMT_OUT);
				M_obj->type = API->GMT->current.setting.export_type;	/* Use selected data type for export */
				M_obj->dim = M_obj->n_columns = col;	/* If COL_FORMAT the dim will change in end_io_matrix after transpose */
				size *= M_obj->n_columns;	/* Size in bytes of the initial matrix allocation */
				if ((error = gmtlib_alloc_univector (API->GMT, &(M_obj->data), M_obj->type, size)) != GMT_OK) return (gmtapi_report_error (API, error));
				M_obj->alloc_mode = GMT_ALLOC_INTERNALLY;
				S_obj->resource = M_obj;	/* Save so we can get it next time */
			}
			M_obj = S_obj->resource;
			GMT_2D_to_index = api_get_2d_to_index (API, GMT_IS_ROW_FORMAT, GMT_GRID_IS_REAL);	/* Since we cannot do col_format without knowing dimension - see end_io_matrix */
			api_put_val = api_select_put_function (API, M_obj->type);
			if (mode == GMT_WRITE_SEGMENT_HEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {	/* Segment header - flag in data as NaNs in current_record (d) */
				for (col = 0; col < M_obj->n_columns; col++) {	/* Place the output items */
					ij = GMT_2D_to_index (S_obj->rec, col, M_obj->dim);
					api_put_val (&(M_obj->data), ij, API->GMT->session.d_NaN);
				}
				S_obj->rec++;					/* Since the NaN-record is an actual data record that encodes a segment break */
				API->current_rec[GMT_OUT]++;	/* Since the NaN-record becomes an actual data record that encodes a segment break */
				M_obj->n_rows++;		/* Same */
			}
			else if (mode == GMT_WRITE_DATA) {	/* Data record */
				if (!record)
					GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: GMT_Put_Record passed a NULL data pointer for method GMT_IS_DUPLICATE_VIA_MATRIX\n");
				else {
					d = record;	/* Cast the void record to a double pointer */
					if (gmt_skip_output (API->GMT, d, M_obj->n_columns))	/* Record was skipped via -s[a|r] */
						error = GMT_NOTSET;
					else {
						for (col = 0; col < M_obj->n_columns; col++) {	/* Place the output items */
							ij = GMT_2D_to_index (S_obj->rec, col, M_obj->dim);
							value = api_select_record_value (API->GMT, d, (unsigned int)col, (unsigned int)API->GMT->common.b.ncol[GMT_OUT]);
							api_put_val (&(M_obj->data), ij, value);
						}
						M_obj->n_rows++;	/* Note that API->current_rec[GMT_OUT] and S_obj->rec are incremented separately at end of function */
					}
				}
			}
			break;

		case GMT_IS_DUPLICATE|GMT_VIA_VECTOR:	/* List of column arrays */
		case GMT_IS_REFERENCE|GMT_VIA_VECTOR:
			if (S_obj->n_rows && S_obj->rec >= S_obj->n_rows)
				GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: GMT_Put_Record exceeding limits on rows(?) - possible bug\n");
			if (S_obj->resource == NULL) {	/* First time allocating space; S_obj->n_rows == S_obj->n_alloc == 0 */
				col = (API->GMT->common.o.active) ? API->GMT->common.o.n_cols : API->GMT->common.b.ncol[GMT_OUT];	/* Number of columns needed to hold the data records */
				if (col == 0 && mode == GMT_WRITE_SEGMENT_HEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {
					/* Cannot place the NaN records since we don't know the number of columns yet */
					S_obj->delay++;
					S_obj->rec++;					/* Since the NaN-record is an actual data record that encodes a segment break */
					API->current_rec[GMT_OUT]++;	/* Since the NaN-record is an actual data record that encodes a segment break */
					break;
				}
				S_obj->n_alloc = GMT_CHUNK;	/* Size in bytes of the initial matrix allocation */
				if ((V_obj = gmt_create_vector (API->GMT, col, GMT_OUT)) == NULL)
					return_error (API, GMT_MEMORY_ERROR);
				for (col = 0; col < V_obj->n_columns; col++)	/* Set same export data type for all vectors */
					V_obj->type[col] = API->GMT->current.setting.export_type;
				if ((error = api_alloc_vectors (API->GMT, V_obj, S_obj->n_alloc)) != GMT_OK) return (gmtapi_report_error (API, error));
				S_obj->resource = V_obj;	/* Save so we can get it next time */
			}
			V_obj = S_obj->resource;

			api_put_val = api_select_put_function (API, V_obj->type[0]);	/* For 1st column and probably all of them - if not we reset below */
			if (mode == GMT_WRITE_SEGMENT_HEADER && API->GMT->current.io.multi_segments[GMT_OUT]) {	/* Segment header - flag in data as NaNs */
				for (col = 0; col < V_obj->n_columns; col++) {	/* Place the output items */
					if (col && V_obj->type[col] != V_obj->type[col-1]) api_put_val = api_select_put_function (API, V_obj->type[col]);
					api_put_val (&(V_obj->data[col]), S_obj->rec, API->GMT->session.d_NaN);
				}
				API->current_rec[GMT_OUT]++;	/* Since the NaN-record is an actual data record that encodes a segment break */
				S_obj->rec++;					/* Since the NaN-record is an actual data record that encodes a segment break */
				V_obj->n_rows++;		/* Same */
			}
			else if (mode == GMT_WRITE_DATA) {	/* Data record */
				if (!record)
					GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: GMT_Put_Record passed a NULL data pointer for method GMT_IS_DATASET_ARRAY\n");
				else {
					d = record;	/* Cast the void record to a double pointer */
					if (gmt_skip_output (API->GMT, d, V_obj->n_columns))	/* Record was skipped via -s[a|r] */
						error = GMT_NOTSET;
					else {
						for (col = 0; col < V_obj->n_columns; col++) {	/* Place the output items */
							value = api_select_record_value (API->GMT, d, (unsigned int)col, (unsigned int)API->GMT->common.b.ncol[GMT_OUT]);
							if (col && V_obj->type[col] != V_obj->type[col-1]) api_put_val = api_select_put_function (API, V_obj->type[col]);
							api_put_val (&(V_obj->data[col]), S_obj->rec, value);
						}
						V_obj->n_rows++;	/* Note that API->current_rec[GMT_OUT] and S_obj->rec are incremented separately at end of function */
					}
				}
			}
			break;

		default:
			GMT_Report (API, GMT_MSG_NORMAL, "GMTAPI: Internal error: GMT_Put_Record called with illegal method\n");
			return_error (API, GMT_NOT_A_VALID_METHOD);
			break;
	}

	if (!error && (mode == GMT_WRITE_DATA || mode == GMT_WRITE_TEXT))	{	/* Only increment if we placed a data record on the output */
		API->current_rec[GMT_OUT]++;
		S_obj->rec++;
	}

	if (S_obj->n_alloc && S_obj->rec == S_obj->n_alloc) {	/* Must allocate more memory for vectors or matrices */
		S_obj->n_alloc <<= 1;
		if (M_obj && (S_obj->method == GMT_IS_DUPLICATE || S_obj->method == GMT_IS_REFERENCE) && S_obj->actual_family == GMT_IS_MATRIX) {
			size_t size = S_obj->n_alloc * M_obj->n_columns;	/* Only one layer in this context */
			if ((error = gmtlib_alloc_univector (API->GMT, &(M_obj->data), M_obj->type, size)) != GMT_OK) return (error);
		}
		else if (V_obj) {	/* VIA_VECTOR */
			if ((error = api_alloc_vectors (API->GMT, V_obj, S_obj->n_alloc)) != GMT_OK) return (error);
		}
	}
	S_obj->status = GMT_IS_USING;	/* Have started writing to this destination */

	return ((error) ? GMT_NOTSET : 0);
}

#ifdef FORTRAN_API
int GMT_Put_Record_ (unsigned int *mode, void *record) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Put_Record (GMT_FORTRAN, *mode, record));
}
#endif

/*! . */
int GMT_Get_Row (void *V_API, int row_no, struct GMT_GRID *G, float *row) {
	/* Reads the entire row vector form the grdfile
	 * If row_no is NEGATIVE it is interpreted to mean that we want to
	 * fseek to the start of the abs(row_no) record and no reading takes place.
	 * If R->auto_advance is false we must set R->start explicitly to row_no.
	 * If R->auto_advance is true it reads the current row and advances R->row++
	 * In this case row_no is not used.
	 */
	unsigned int err;
 	unsigned int col;
	struct GMTAPI_CTRL *API = NULL;
	char *fmt = NULL;
	struct GMT_GRID_ROWBYROW *R = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	GMT = API->GMT;
	fmt = GMT->session.grdformat[G->header->type];
	R = api_get_rbr_ptr (G->extra);
	if (fmt[0] == 'c') {		/* Get one NetCDF row, old format */
		if (row_no < 0) {	/* Special seek instruction, then return */
			R->row = abs (row_no);
			R->start[0] = R->row * R->edge[0];
			return (GMT_NOERROR);
		}
		else if (!R->auto_advance) {	/* Go to specified row and read it */
			R->row = row_no;
			R->start[0] = R->row * R->edge[0];
		}
		gmt_M_err_trap (nc_get_vara_float (R->fid, G->header->z_id, R->start, R->edge, row));
		if (R->auto_advance) R->start[0] += R->edge[0];	/* Advance to next row if auto */
	}
	else if (fmt[0] == 'n') {	/* Get one NetCDF row, COARDS-compliant format */
		if (row_no < 0) {	/* Special seek instruction */
			R->row = abs (row_no);
			R->start[0] = G->header->row_order == k_nc_start_north ? R->row : G->header->n_rows - 1 - R->row;
			return (GMT_NOERROR);
		}
		else if (!R->auto_advance) {
			R->row = row_no;
			R->start[0] = G->header->row_order == k_nc_start_north ? R->row : G->header->n_rows - 1 - R->row;
		}
		gmt_M_err_trap (nc_get_vara_float (R->fid, G->header->z_id, R->start, R->edge, row));
		if (R->auto_advance) R->start[0] -= G->header->row_order;	/* Advance to next row if auto */
	}
	else {			/* Get a native binary row */
		size_t n_items;
		if (row_no < 0) {	/* Special seek instruction */
			R->row = abs (row_no);
			if (fseek (R->fp, (off_t)(GMT_GRID_HEADER_SIZE + R->row * R->n_byte), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
			return (GMT_NOERROR);
		}
		R->row = row_no;
		if (!R->auto_advance && fseek (R->fp, (off_t)(GMT_GRID_HEADER_SIZE + R->row * R->n_byte), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);

		n_items = G->header->n_columns;
		if (fmt[1] == 'f') {	/* Binary float, no need to mess with decoding */
			if (gmt_M_fread (row, R->size, n_items, R->fp) != n_items) return (GMT_GRDIO_READ_FAILED);	/* Get one row */
		}
		else {
			if (gmt_M_fread (R->v_row, R->size, n_items, R->fp) != n_items) return (GMT_GRDIO_READ_FAILED);	/* Get one row */
			for (col = 0; col < G->header->n_columns; col++)
				row[col] = gmtlib_decode (GMT, R->v_row, col, fmt[1]);	/* Convert whatever to float */
		}
	}
	if (R->check) {	/* Replace NaN-marker with actual NaN */
		for (col = 0; col < G->header->n_columns; col++)
			if (row[col] == G->header->nan_value)
				row[col] = GMT->session.f_NaN;
	}
	gmt_scale_and_offset_f (GMT, row, G->header->n_columns, G->header->z_scale_factor, G->header->z_add_offset);
	if (R->auto_advance) R->row++;
	return (GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Get_Row_ (int *rec_no, struct GMT_GRID *G, float *row) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Row (GMT_FORTRAN, *rec_no, G, row));
}
#endif

/*! . */
int GMT_Put_Row (void *V_API, int rec_no, struct GMT_GRID *G, float *row) {
	/* Writes the entire row vector to the grdfile
	 * If row_no is NEGATIVE it is interpreted to mean that we want to
	 * fseek to the start of the abs(row_no) record and no reading takes place.
	 * If R->auto_advance is false we must set R->start explicitly to row_no.
	 * If R->auto_advance is true it writes at the current row and advances R->row++
	 * In this case row_no is not used.
	 */

	unsigned int err;	/* Required by gmt_M_err_trap */
	unsigned int col;
	size_t n_items;
	struct GMTAPI_CTRL *API = NULL;
	char *fmt = NULL;
	struct GMT_GRID_ROWBYROW *R = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	GMT = API->GMT;
	fmt = GMT->session.grdformat[G->header->type];
	R = api_get_rbr_ptr (G->extra);
	gmt_scale_and_offset_f (GMT, row, G->header->n_columns, G->header->z_scale_factor, G->header->z_add_offset);
	if (R->check) {	/* Replace NaNs with special value */
		for (col = 0; col < G->header->n_columns; col++)
			if (gmt_M_is_fnan (row[col]))
				row[col] = G->header->nan_value;
	}

	switch (fmt[0]) {
		case 'c':
			if (!R->auto_advance) R->start[0] = rec_no * R->edge[0];
			gmt_M_err_trap (nc_put_vara_float (R->fid, G->header->z_id, R->start, R->edge, row));
			if (R->auto_advance) R->start[0] += R->edge[0];
			break;
		case 'n':
			if (!R->auto_advance) R->start[0] = G->header->row_order = k_nc_start_north ? rec_no : G->header->n_rows - 1 - rec_no;
			gmt_M_err_trap (nc_put_vara_float (R->fid, G->header->z_id, R->start, R->edge, row));
			if (R->auto_advance) R->start[0] -= G->header->row_order;
			break;
		default:
			if (!R->auto_advance && fseek (R->fp, (off_t)(GMT_GRID_HEADER_SIZE + rec_no * R->n_byte), SEEK_SET)) return (GMT_GRDIO_SEEK_FAILED);
			n_items = G->header->n_columns;
			if (fmt[1] == 'f') {	/* Regular floats */
				if (gmt_M_fwrite (row, R->size, n_items, R->fp) < n_items) return (GMT_GRDIO_WRITE_FAILED);
			}
			else {
				for (col = 0; col < G->header->n_columns; col++) gmtlib_encode (GMT, R->v_row, col, row[col], fmt[1]);
				if (gmt_M_fwrite (R->v_row, R->size, n_items, R->fp) < n_items) return (GMT_GRDIO_WRITE_FAILED);
			}
			break;
	}
	if (R->auto_advance) R->row++;

	return (GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Put_Row_ (int *rec_no, struct GMT_GRID *G, float *row) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Put_Row (GMT_FORTRAN, *rec_no, G, row));
}
#endif

GMT_LOCAL char *ptrvoid (char ** p) { 	/* Handle as char ** just to determine if address is of a NULL pointer */
	return *p;
}

/*! . */
int GMT_Destroy_Data (void *V_API, void *object) {
	/* Destroy a resource that is no longer needed.
	 * Returns the error code.
	 */
	int error, item, object_ID;
	enum GMT_enum_family family;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);	/* This is a cardinal sin */
	if (object == NULL) return (false);	/* Null address, quietly skip */
	if (!ptrvoid(object)) return (false);	/* Null pointer, quietly skip */
	API = api_get_api_ptr (V_API);		/* Now we need to get that API pointer to check further */
	if ((object_ID = api_get_object_id_from_data_ptr (API, object)) == GMT_NOTSET) return_error (API, GMT_OBJECT_NOT_FOUND);	/* Could not find the object in the list */
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET) return_error (API, API->error);	/* Could not find that item */
	family = API->object[item]->actual_family;

	switch (family) {	/* Standard 6 families, plus matrix/vector and coordinates */
		case GMT_IS_GRID:	/* GMT grid */
			error = api_destroy_grid (API, object);
			break;
		case GMT_IS_DATASET:
			error = api_destroy_dataset (API, object);
			break;
		case GMT_IS_TEXTSET:
			error = api_destroy_textset (API, object);
			break;
		case GMT_IS_PALETTE:
			error = api_destroy_palette (API, object);
			break;
		case GMT_IS_IMAGE:
			error = api_destroy_image (API, object);
			break;
		case GMT_IS_POSTSCRIPT:
			error = api_destroy_postscript (API, object);
			break;

		/* Also allow destroying of intermediate vector and matrix containers */
		case GMT_IS_MATRIX:
			error = api_destroy_matrix (API, object);
			break;
		case GMT_IS_VECTOR:
			error = api_destroy_vector (API, object);
			break;
		case GMT_IS_COORD:
			error = api_destroy_coord (API, object);
			break;
		default:
			return_error (API, GMT_NOT_A_VALID_FAMILY);
			break;
	}
	if (!error) {	/* We successfully freed the items, now remove from IO list */
		unsigned int j;
		void *address = API->object[item]->data;
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Destroy_Data: freed memory for a %s for object %d\n", GMT_family[family], object_ID);
		if ((error = gmtapi_unregister_io (API, object_ID, (unsigned int)GMT_NOTSET))) return_error (API, error);	/* Did not find object */
		for (j = 0; j < API->n_objects; j++) {
			if (API->object[j]->data == address) API->object[j]->data = NULL;		/* Set repeated data references to NULL so we don't try to free twice */
			if (API->object[j]->resource == address) API->object[j]->resource = NULL;	/* Set matching resources to NULL so we don't try to read from there again */
		}
#ifdef DEBUG
		api_list_objects (API, "GMT_Destroy_Data");
#endif

	}
	else {
		/* Quietly ignore these errors: GMT_PTR_IS_NULL, GMT_FREE_EXTERNAL_NOT_ALLOWED, GMT_FREE_WRONG_LEVEL as they are not considered errors here. */
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Destroy_Data: Ignored warning %d for object %d\n", error, object_ID);
	}
	return_error (API, GMT_OK);
}

#ifdef FORTRAN_API
int GMT_Destroy_Data_ (void *object) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Destroy_Data (GMT_FORTRAN, object));
}
#endif

int api_destroy_grids (struct GMTAPI_CTRL *API, struct GMT_GRID ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_GRID **G = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &G[k]))) return_error (API, error);
	gmt_M_free (API->GMT, G);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_datasets (struct GMTAPI_CTRL *API, struct GMT_DATASET ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_DATASET **D = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &D[k]))) return_error (API, error);
	gmt_M_free (API->GMT, D);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_textsets (struct GMTAPI_CTRL *API, struct GMT_TEXTSET ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_TEXTSET **T = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &T[k]))) return_error (API, error);
	gmt_M_free (API->GMT, T);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_images (struct GMTAPI_CTRL *API, struct GMT_IMAGE ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_IMAGE **I = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &I[k]))) return_error (API, error);
	gmt_M_free (API->GMT, I);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_palettes (struct GMTAPI_CTRL *API, struct GMT_PALETTE ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_PALETTE **C = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &C[k]))) return_error (API, error);
	gmt_M_free (API->GMT, C);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_postscripts (struct GMTAPI_CTRL *API, struct GMT_POSTSCRIPT ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_POSTSCRIPT **P = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &P[k]))) return_error (API, error);
	gmt_M_free (API->GMT, P);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_matrices (struct GMTAPI_CTRL *API, struct GMT_MATRIX ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_MATRIX **M = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &M[k]))) return_error (API, error);
	gmt_M_free (API->GMT, M);	*obj = NULL;
	return_error (API, GMT_OK);
}

int api_destroy_vectors (struct GMTAPI_CTRL *API, struct GMT_VECTOR ***obj, unsigned int n_items)
{
	unsigned int k;
	int error;
	struct GMT_VECTOR **V = *obj;
	for (k = 0; k < n_items; k++) if ((error = GMT_Destroy_Data (API, &V[k]))) return_error (API, error);
	gmt_M_free (API->GMT, V);	*obj = NULL;
	return_error (API, GMT_OK);
}

void **void3_to_void2 (void ***p) { return (*p); }

/*! . */
int GMT_Destroy_Group (void *V_API, void *object, unsigned int n_items) {
	/* Destroy an array of resources that are no longer needed.
	 * Returns the error code.
	 */
	int error, object_ID, item;
	void **ptr = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);	/* This is a cardinal sin */
	if (object == NULL) return (false);	/* Null address, quietly skip */
	API = api_get_api_ptr (V_API);		/* Now we need to get that API pointer to check further */
	ptr = void3_to_void2 (object);		/* Get the array of pointers */
	if ((object_ID = api_get_object_id_from_data_ptr (API, ptr)) == GMT_NOTSET) return_error (API, GMT_OBJECT_NOT_FOUND);	/* Could not find the object in the list */
	if ((item = gmtapi_validate_id (API, GMT_NOTSET, object_ID, GMT_NOTSET, GMT_NOTSET)) == GMT_NOTSET) return_error (API, API->error);	/* Could not find that item */
	switch (API->object[item]->actual_family) {
		case GMT_IS_GRID:       error = api_destroy_grids    (API, object, n_items); break;
		case GMT_IS_DATASET:    error = api_destroy_datasets (API, object, n_items); break;
		case GMT_IS_TEXTSET:    error = api_destroy_textsets (API, object, n_items); break;
		case GMT_IS_IMAGE:      error = api_destroy_images   (API, object, n_items); break;
		case GMT_IS_PALETTE:    error = api_destroy_palettes (API, object, n_items); break;
		case GMT_IS_POSTSCRIPT: error = api_destroy_postscripts      (API, object, n_items); break;
		case GMT_IS_MATRIX:     error = api_destroy_matrices (API, object, n_items); break;
		case GMT_IS_VECTOR:     error = api_destroy_vectors  (API, object, n_items); break;
		default: return_error (API, GMT_NOT_A_VALID_FAMILY); break;
	}
	return_error (API, error);
}

#ifdef FORTRAN_API
int GMT_Destroy_Group_ (void *object, unsigned int *n_items) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Destroy_Group (GMT_FORTRAN, object, *n_items));
}
#endif

/*! . */
void *GMT_Create_Data (void *V_API, unsigned int family, unsigned int geometry, unsigned int mode, uint64_t dim[], double *range, double *inc, unsigned int registration, int pad, void *data) {
	/* Create an empty container of the requested kind and allocate space for content.
	 * The known families are GMT_IS_{DATASET,TEXTSET,GRID,PALETTE,IMAGE,POSTSCRIPT}, but we
	 * also allow for creation of the containers for GMT_IS_{VECTOR,MATRIX}. Note
	 * that for VECTOR|MATRIX we don't allocate space to hold data as it is the users
	 * responsibility to hook their data pointers in.  The VECTOR allocates the array
	 * of column vector type and data pointers.
	 * Geometry should reflect the resource, e.g. GMT_IS_SURFACE for grid, etc.
	 * There are two ways to define the dimensions needed to actually allocate memory:
	 * (A) Via uint64_t dim[]:
	 *     The dim array contains up to 4 dimensions for:
	 *	   0: dim[GMT_TBL] = number of tables,
	 *	   1: dim[GMT_SEG] = number of segments per table
	 *	   2: dim[GMT_ROW] = number of rows per segment.
	 *	   3: dim[GMT_COL] = number of columns per row [ignored for GMT_TEXTSET].
	 *     The dim array is ignored for CPTs.
	 *     For GMT_IS_IMAGE & GMT_IS_MATRIX, par[GMT_Z] = GMT[2] holds the number of bands or layers (dim == NULL means just 1).
	 *     For GMT_IS_GRID, GMT_IS_IMAGE, & GMT_IS_MATRIX: dim[0] holds the number of columns and dim[1] holds the number
	 *         of rows; this implies that wesn = 0-<dim-1>, inc = 1, and registration is pixel-registration.
	 *     For GMT_IS_VECTOR, dim[0] holds the number of columns, optionally dim[1] holds number of rows, if known, or 0.
	 *	   dim[2] can hold the data type (GMT_DOUBLE, etc). If dim[1] > 0 then we allocate the rows.
	 * (B) Via range, inc, registration:
	 *     Convert user domain range, increments, and registration into dimensions
	 *     for the container.  For grids and images we fill out the GMT_GRID_HEADER;
	 *     for vectors and matrices we fill out their internal parameters.
	 *     For complex grids pass registration + GMT_GRID_IS_COMPLEX_{REAL|IMAG}
	 *     For GMT_IS_MATRIX and GMT_IS_IMAGE, par[GMT_Z] = holds the number of layers or bands (dim == NULL means just 1).
	 * pad sets the padding for grids and images, ignored for other resources.
	 * Some default actions for grids:
	 * range = NULL: Select current -R setting if present.
	 * registration = GMT_NOTSET: Gridline unless -r is in effect.
	 * Give -1 (GMT_NOTSET) to accept GMT default padding [2].
	 *
	 * For creating grids and images you can do it in one or two steps:
 	 * (A) Pass mode = GMT_CONTAINER_AND_DATA; this creates both header and allocates grid|image;
	 * (B) Call GMT_Create_Data twice:
	 * 	1. First with mode = GMT_CONTAINER_ONLY which creates header only
	 *	   and computes the dimensions based on the other arguments.
	 *	2. 2nd with mode = GMT_DATA_ONLY, which allocates the grid|image array
	 *	   based on the dimensions already set.  This time you pass NULL/0
	 *	   for dim, wesn, inc, registration, pad but let data be your grid|image
	 *	   returned to you after step 1.
	 *
	 * By default, the created resource is consider an input resource (direction == GMT_IN).
	 * However, for the interface containers GMT_VECTOR and GMT_MATRIX they will have their
	 * direction set to GMT_OUT if the row-dimension is not set.
	 *
	 * Return: Pointer to resource, or NULL if an error (set via API->error).
	 */

	int error = GMT_OK;
	int def_direction = GMT_IN;	/* Default direction is GMT_IN  */
	unsigned int module_input;
	uint64_t n_layers = 0, zero_dim[4] = {0, 0, 0, 0}, *this_dim = dim;
	bool already_registered = false, has_ID = false;
	void *new_obj = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if (mode == GMT_IS_OUTPUT) {	/* Flagged to be an output container */
		def_direction = GMT_OUT;	/* Set output as default direction*/
		this_dim = zero_dim;		/* Provide dimensions set to zero */
		if (data) return_null (API, GMT_PTR_NOT_NULL);	/* Error if data pointer is not NULL */
	}

	module_input = (family & GMT_VIA_MODULE_INPUT);	/* Are we creating a resource that is a module input? */
	family -= module_input;

	/* Below, data can only be non-NULL for Grids or Images passing back G or I to allocate the data array */

	switch (family) {	/* dataset, cpt, text, grid , image, vector, matrix */
		case GMT_IS_GRID:	/* GMT grid, allocate header but not data array */
			if (mode == GMT_IS_OUTPUT || (mode & GMT_DATA_ONLY) == 0) {	/* Create new grid unless we only ask for data only */
				if (data) return_null (API, GMT_PTR_NOT_NULL);	/* Error if data pointer is not NULL */
	 			if ((new_obj = gmt_create_grid (API->GMT)) == NULL)
	 				return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
				if (pad != GMT_NOTSET) gmt_set_pad (API->GMT, pad);	/* Change the default pad; give -1 to leave as is */
				if ((error = api_init_grid (API, NULL, this_dim, range, inc, registration, mode, def_direction, new_obj)))
					return_null (API, error);
				if (pad != GMT_NOTSET) gmt_set_pad (API->GMT, API->pad);	/* Reset to the default pad */
			}
			else {	/* Already registered so has_ID must be false */
				if (has_ID || (new_obj = data) == NULL)
					return_null (API, GMT_PTR_IS_NULL);	/* Error if data pointer is NULL */
				already_registered = true;
			}
			if (def_direction == GMT_IN && (mode & GMT_CONTAINER_ONLY) == 0) {	/* Allocate the grid array unless we asked for header only */
				if ((error = api_alloc_grid (API->GMT, new_obj)) != GMT_NOERROR)
					return_null (API, error);	/* Allocation error */
			}
			if (mode & GMT_GRID_XY) {	/* Also allocate and populate the x,y vectors */
				if ((error = api_alloc_grid_xy (API, new_obj)) != GMT_NOERROR)
					return_null (API, error);	/* Allocation error */
			}
			break;
		case GMT_IS_IMAGE:	/* GMT image, allocate header but not data array */
			if (mode == GMT_IS_OUTPUT || (mode & GMT_DATA_ONLY) == 0) {	/* Create new image unless we only ask for data only */
				if (data) return_null (API, GMT_PTR_NOT_NULL);	/* Error if data is not NULL */
	 			if ((new_obj = gmtlib_create_image (API->GMT)) == NULL)
	 				return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
				if (pad != GMT_NOTSET) gmt_set_pad (API->GMT, pad);	/* Change the default pad; give -1 to leave as is */
				if ((error = api_init_image (API, NULL, this_dim, range, inc, registration, mode, def_direction, new_obj)))
					return_null (API, error);
				if (pad != GMT_NOTSET) gmt_set_pad (API->GMT, API->pad);	/* Reset to the default pad */
			}
			else {
				if ((new_obj = data) == NULL)
					return_null (API, GMT_PTR_IS_NULL);	/* Error if data is NULL */
				already_registered = true;
			}
			if (def_direction == GMT_IN && (mode & GMT_CONTAINER_ONLY) == 0) {	/* Allocate the image array unless we asked for header only */
				if ((error = api_alloc_image (API->GMT, dim, new_obj)) != GMT_NOERROR)
					return_null (API, error);	/* Allocation error */
			}
			if (mode & GMT_GRID_XY) {	/* Also allocate and populate the image x,y vectors */
				if ((error = api_alloc_image_xy (API, new_obj)) != GMT_NOERROR)
					return_null (API, error);	/* Allocation error */
			}
			break;
		case GMT_IS_DATASET:	/* GMT dataset, allocate the requested tables, segments, rows, and columns */
			if (this_dim[GMT_TBL] > UINT_MAX || this_dim[GMT_ROW] > UINT_MAX)
				return_null (API, GMT_DIM_TOO_LARGE);
			if ((new_obj = gmtlib_create_dataset (API->GMT, this_dim[GMT_TBL], this_dim[GMT_SEG], this_dim[GMT_ROW], this_dim[GMT_COL], geometry, false)) == NULL)
				return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
			break;
		case GMT_IS_TEXTSET:	/* GMT text dataset, allocate the requested tables, segments, and rows */
			if (this_dim[GMT_TBL] > UINT_MAX) return_null (API, GMT_DIM_TOO_LARGE);
			if ((new_obj = gmtlib_create_textset (API->GMT, this_dim[GMT_TBL], this_dim[GMT_SEG], this_dim[GMT_ROW], false)) == NULL)
				return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
			break;
		case GMT_IS_PALETTE:	/* GMT CPT, allocate one with space for dim[0] color entries */
			/* If dim is NULL then we ask for 0 color entries as direction here is GMT_OUT for return to an external API */
		 	if ((new_obj = gmtlib_create_palette (API->GMT, this_dim[0])) == NULL)
		 		return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
			break;
		case GMT_IS_POSTSCRIPT:	/* GMT PS struct, allocate one struct */
		 	if ((new_obj = gmtlib_create_ps (API->GMT, this_dim[0])) == NULL)
		 		return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */
			break;
		case GMT_IS_MATRIX:	/* GMT matrix container, allocate one with the requested number of layers, rows & columns */
			n_layers = (this_dim[GMTAPI_DIM_COL] == 0 && this_dim[GMTAPI_DIM_ROW] == 0) ? 1U : this_dim[GMT_Z];
		 	new_obj = gmtlib_create_matrix (API->GMT, n_layers, def_direction);
			if (pad) GMT_Report (API, GMT_MSG_VERBOSE, "Pad argument (%d) ignored in initialization of %s\n", pad, GMT_family[family]);
			if ((API->error = api_init_matrix (API, this_dim, range, inc, registration, mode, def_direction, new_obj))) {	/* Failure, must free the object */
				struct GMT_MATRIX *M = api_return_address (new_obj, GMT_IS_MATRIX);	/* Get pointer to resource */
				gmtlib_free_matrix (API->GMT, &M, true);
			}
			break;
		case GMT_IS_VECTOR:	/* GMT vector container, allocate one with the requested number of columns & rows */
			if (dim && dim[GMTAPI_DIM_COL] == 0) return_null (API, GMT_N_COLS_NOT_SET);
	 		new_obj = gmt_create_vector (API->GMT, this_dim[GMTAPI_DIM_COL], def_direction);
			if (pad) GMT_Report (API, GMT_MSG_VERBOSE, "Pad argument (%d) ignored in initialization of %s\n", pad, GMT_family[family]);
			if ((API->error = api_init_vector (API, this_dim, range, inc, registration, mode, def_direction, new_obj))) {	/* Failure, must free the object */
				struct GMT_VECTOR *V = api_return_address (new_obj, GMT_IS_VECTOR);	/* Get pointer to resource */
				gmt_free_vector (API->GMT, &V, true);
			}
			break;
		default:
			API->error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	if (API->error) return_null (API, API->error);	/* Failed to pass the right arguments */

	if (!already_registered) {	/* Register this object so it can be deleted by GMT_Destroy_Data or gmtapi_garbage_collection */
		enum GMT_enum_method method = GMT_IS_REFERENCE;	/* Since it is a memory object */
		int item = GMT_NOTSET, object_ID = GMT_NOTSET;
		if ((object_ID = GMT_Register_IO (API, family|module_input, method, geometry, def_direction, range, new_obj)) == GMT_NOTSET)
			return_null (API, API->error);	/* Failure to register */
		if ((item = gmtapi_validate_id (API, family, object_ID, def_direction, GMT_NOTSET)) == GMT_NOTSET)
			return_null (API, API->error);
		API->object[item]->data = new_obj;	/* Retain pointer to the allocated data so we use garbage collection later */
		API->object[item]->actual_family = family;	/* No distinction between these at this point since usage is unknown */
		if (def_direction == GMT_OUT) API->object[item]->messenger = true;	/* We are passing a dummy container that should be destroyed before returning actual data */
		GMT_Report (API, GMT_MSG_DEBUG, "Successfully created a new %s container\n", GMT_family[family]);
	}
	else
		GMT_Report (API, GMT_MSG_DEBUG, "Successfully added data array to previously registered %s container\n", GMT_family[family]);

	return (new_obj);
}

#ifdef FORTRAN_API
void *GMT_Create_Data_ (unsigned int *family, unsigned int *geometry, unsigned int *mode, uint64_t *dim, double *range, double *inc, unsigned int *registration, int *pad, void *container) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Create_Data (GMT_FORTRAN, *family, *geometry, *mode, dim, range, inc, *registration, *pad, container));
}
#endif

/*! Convenience function to get grid or image node */
uint64_t GMT_Get_Index (void *V_API, struct GMT_GRID_HEADER *header, int row, int col) {
	/* V_API not used but all API functions take V_API so no exceptions! */
	gmt_M_unused(V_API);
	return (header->index_function (header, row, col, 0));
}

#ifdef FORTRAN_API
uint64_t GMT_Get_Index_ (void *h, int *row, int *col) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Index (GMT_FORTRAN, h, *row, *col));
}
#endif

/*! Convenience function to get grid or image node */
uint64_t GMT_Get_Pixel (void *V_API, struct GMT_GRID_HEADER *header, int row, int col, int layer) {
	/* V_API not used but all API functions take V_API so no exceptions! */
	gmt_M_unused(V_API);
	return (header->index_function (header, row, col, layer));
}

#ifdef FORTRAN_API
uint64_t GMT_Get_Pixel_ (void *h, int *row, int *col, int *layer) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Pixel (GMT_FORTRAN, h, *row, *col, *layer));
}
#endif

/*! Specify image memory layout */
int GMT_Set_Index (void *V_API, struct GMT_GRID_HEADER *header, char *code) {
	struct GMTAPI_CTRL *API = NULL;
	enum GMT_enum_family family;
	unsigned int mode;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	mode = api_decode_layout (API, code, &family);
	switch (family) {
		case GMT_IS_GRID:
			switch (mode) {
				case 0:	/* Default scanline C grid */
					header->index_function = api_get_index_from_TRS;
					break;
				case 4:	/* Same for real component in complex grid */
					header->index_function = api_get_index_from_TRR;
					break;
				case 8:	/* Same for imag component in complex grid */
					header->index_function = api_get_index_from_TRI;
					break;
				default:
					GMT_Report (API, GMT_MSG_NORMAL, "Unrecognized mode for grid layout [%u]\n", mode);
					API->error = GMT_NOT_A_VALID_MODULE;
					break;
			}
			break;
		case GMT_IS_IMAGE:
			switch (mode) {
				case 0:	/* band-interleaved layout */
					header->index_function = api_get_index_from_TRB;
					break;
				case 4:	/* pixel-interleaved layout */
					header->index_function = api_get_index_from_TRP;
					break;
				case 8:	/* line-interleaved layout */
					header->index_function = api_get_index_from_TRL;
					break;
				default:
					GMT_Report (API, GMT_MSG_NORMAL, "Unrecognized mode for image layout [%u]\n", mode);
					API->error = GMT_NOT_A_VALID_MODULE;
					break;
			}
			break;
		default:
			GMT_Report (API, GMT_MSG_NORMAL, "Unrecognized family for api_decode_layout [%s]\n", code);
			API->error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	return API->error;
}

#ifdef FORTRAN_API
int GMT_Set_Index_ (void *h, int *mode) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Set_Index (GMT_FORTRAN, h, *mode));
}
#endif

/*! . */
double *GMT_Get_Coord (void *V_API, unsigned int family, unsigned int dim, void *container) {
	/* Return an array of coordinates for the nodes along the specified dimension.
	 * For GMT_GRID and GMT_IMAGE, dim is either 0 (GMT_X) or 1 (GMT_Y) while for
	 * GMT_MATRIX it may be 2 (GMT_Z), provided the matrix has more than 1 layer.
	 * For GMT_VECTOR that was registered as equidistant it will return coordinates
	 * along the single dimension.
	 * Cannot be used on other resources (GMT_DATASET, GMT_TEXTSET, GMT_PALETTE).
	 */
	int object_ID, item;
	double *coord = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	if (container == NULL) return_null (V_API, GMT_ARG_IS_NULL);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	switch (family) {	/* grid, image, or matrix */
		case GMT_IS_GRID:	/* GMT grid */
			if (dim > GMT_Y) return_null (API, GMT_DIM_TOO_LARGE);
			coord = api_grid_coord (API, dim, container);
			break;
		case GMT_IS_IMAGE:	/* GMT image */
			if (dim > GMT_Y) return_null (API, GMT_DIM_TOO_LARGE);
			coord = api_image_coord (API, dim, container);
			break;
		case GMT_IS_VECTOR:	/* GMT vector */
			if (dim != GMT_Y) return_null (API, GMT_DIM_TOO_LARGE);
			coord = api_vector_coord (API, dim, container);
			break;
		case GMT_IS_MATRIX:	/* GMT matrix */
			if (dim > GMT_Z) return_null (API, GMT_DIM_TOO_LARGE);
			coord = api_matrix_coord (API, dim, container);
			break;
		default:
			return_null (API, GMT_NOT_A_VALID_FAMILY);
			break;
	}
	/* We register the coordinate array so that GMT_Destroy_Data can free them later */
	if ((object_ID = GMT_Register_IO (V_API, GMT_IS_COORD, GMT_IS_COORD, GMT_IS_NONE, GMT_IN, NULL, coord)) == GMT_NOTSET)
		return_null (API, API->error);
	if ((item = gmtapi_validate_id (API, GMT_IS_COORD, object_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
		return_null (API, API->error);
	API->object[item]->data = coord;	/* Retain pointer to the allocated data so we use garbage collection later */
	GMT_Report (API, GMT_MSG_DEBUG, "Successfully created a new coordinate array for %s\n", GMT_family[family]);

	return (coord);
}

#ifdef FORTRAN_API
double *GMT_Get_Coord_ (unsigned int *family, unsigned int *dim, void *container) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Coord (GMT_FORTRAN, *family, *dim, container));
}
#endif

/*! . */
int GMT_Set_Comment (void *V_API, unsigned int family, unsigned int mode, void *arg, void *container) {
	/* Set new header comment or grid command|remark to container */

	int error = GMT_OK;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (container == NULL) return_error (V_API, GMT_ARG_IS_NULL);
	if (arg == NULL) return_error (V_API, GMT_ARG_IS_NULL);
	API = api_get_api_ptr (V_API);

	switch (family) {	/* grid, image, or matrix */
		case GMT_IS_GRID:	/* GMT grid */
			api_grid_comment (API, mode, arg, container);
			break;
		case GMT_IS_IMAGE:	/* GMT image */
			api_image_comment (API, mode, arg, container);
			break;
		case GMT_IS_DATASET:	/* GMT dataset */
			api_dataset_comment (API, mode, arg, container);
			break;
		case GMT_IS_TEXTSET:	/* GMT textset */
			api_textset_comment (API, mode, arg, container);
			break;
		case GMT_IS_PALETTE:	/* GMT CPT */
			api_cpt_comment (API, mode, arg, container);
			break;
		case GMT_IS_POSTSCRIPT:		/* GMT PS */
			api_ps_comment (API, mode, arg, container);
			break;
		case GMT_IS_VECTOR:	/* GMT Vector [PW: Why do we need these?]*/
			api_vector_comment (API, mode, arg, container);
			break;
		case GMT_IS_MATRIX:	/* GMT Vector */
			api_matrix_comment (API, mode, arg, container);
			break;
		default:
			error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	return_error (API, error);
}

/* FFT Extension: Functions available to do FFT work within the API */

/*! . */
unsigned int GMT_FFT_Option (void *V_API, char option, unsigned int dim, const char *string) {
	/* For programs that needs to do either 1-D or 2-D FFT work */
	unsigned int d1 = dim - 1;	/* Index into the info text strings below for 1-D (0) and 2-D (1) case */
	char *data_type[2] = {"table", "grid"}, *dim_name[2] = {"<n_columns>", "<n_columns>/<n_rows>"}, *trend_type[2] = {"line", "plane"};
	char *dim_ref[2] = {"dimension", "dimensions"}, *linear_type[2] = {"linear", "planar"};
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (dim > 2) return_error (V_API, GMT_DIM_TOO_LARGE);
	if (dim == 0) return_error (V_API, GMT_DIM_TOO_SMALL);
	if (string && string[0] == ' ') GMT_Report (V_API, GMT_MSG_NORMAL, "Syntax error -%c option.  Correct syntax:\n", option);
	if (string)
		GMT_Message (V_API, GMT_TIME_NONE, "\t-%c %s\n", option, string);
	else
		GMT_Message (V_API, GMT_TIME_NONE, "\t-%c Choose or inquire about suitable %s %s for %u-D FFT, and set modifiers.\n", option, data_type[d1], dim_ref[d1], dim);
	GMT_Message (V_API, GMT_TIME_NONE, "\t   Setting the FFT %s:\n", dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -Na will select %s promising the most accurage results.\n", dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -Nf will force the FFT to use the %s of the %s.\n", dim_ref[d1], data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -Nm will select %s using the least work storage.\n", dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -Nr will select %s promising the most rapid calculation.\n", dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -Ns will list Singleton's [1967] recommended %s, then exit.\n", dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     -N%s will do FFT on array size %s (Must be >= %s size).\n", dim_name[d1], dim_name[d1], data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     Default chooses %s >= %s %s to optimize speed and accuracy of the FFT.\n", dim_ref[d1], data_type[d1], dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t   Append modifiers for removing a %s trend:\n", linear_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +d: Detrend data, i.e., remove best-fitting %s [Default].\n", trend_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +a: Only remove mean value.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +h: Only remove mid value, i.e., 0.5 * (max + min).\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +l: Leave data alone.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t   Append modifiers for extending the %s via symmetries:\n", data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     If FFT %s > %s %s, data are extended via edge point symmetry\n", dim_ref[d1], data_type[d1], dim_ref[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     and tapered to zero.  Several modifiers can be set to change this behavior:\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +e: Extend data via edge point symmetry [Default].\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +m: Extend data via edge mirror symmetry.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +n: Do NOT extend data.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +t<w>: Limit tapering to <w> %% of the extended margins [100].\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     If +n is set then +t instead sets the boundary width of the interior\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     %s margin to be tapered [0].\n", data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t   Append modifiers for saving modified %s before or after the %u-D FFT is called:\n", data_type[d1], dim);
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +w[<suffix>] will write the intermediate %s passed to FFT after detrending/extension/tapering.\n", data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t       File name will have _<suffix> [tapered] inserted before file extension.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +z[p] will write raw complex spectrum to two separate %s files.\n", data_type[d1]);
	GMT_Message (V_API, GMT_TIME_NONE, "\t       File name will have _real/_imag inserted before the file extensions.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t       Append p to store polar forms, using _mag/_phase instead.\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t   Append modifiers for messages:\n");
	GMT_Message (V_API, GMT_TIME_NONE, "\t     +v will report all suitable dimensions (except when -Nf is selected).\n");

	return_error (V_API, GMT_NOERROR);
}

#ifdef FORTRAN_API
unsigned int GMT_FFT_Option_ (char *option, unsigned int *dim, const char *string, int *length) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_FFT_Option (GMT_FORTRAN, *option, *dim, string));
}
#endif

/* first 2 cols from table III of Singleton's paper on fft.... */
#define N_SINGLETON_LIST	117
static int Singleton_list[N_SINGLETON_LIST] = {
	64,72,75,80,81,90,96,100,108,120,125,128,135,144,150,160,162,180,192,200,
	216,225,240,243,250,256,270,288,300,320,324,360,375,384,400,405,432,450,480,
	486,500,512,540,576,600,625,640,648,675,720,729,750,768,800,810,864,900,960,
	972,1000,1024,1080,1125,1152,1200,1215,1250,1280,1296,1350,1440,1458,1500,
	1536,1600,1620,1728,1800,1875,1920,1944,2000,2025,2048,2160,2187,2250,2304,
	2400,2430,2500,2560,2592,2700,2880,2916,3000,3072,3125,3200,3240,3375,3456,
	3600,3645,3750,3840,3888,4000,4096,4320,4374,4500,4608,4800,4860,5000};

GMT_LOCAL void fft_Singleton_list (struct GMTAPI_CTRL *API) {
	unsigned int k;
	char message[GMT_LEN16] = {""};
	GMT_Message (API, GMT_TIME_NONE, "\t\"Good\" numbers for FFT dimensions [Singleton, 1967]:\n");
	for (k = 0; k < N_SINGLETON_LIST; k++) {
		snprintf (message, GMT_LEN16, "\t%d", Singleton_list[k]);
		if ((k+1) % 10 == 0 || k == (N_SINGLETON_LIST-1)) strcat (message, "\n");
		GMT_Message (API, GMT_TIME_NONE, message);
	}
}

/*! . */
void *GMT_FFT_Parse (void *V_API, char option, unsigned int dim, const char *args) {
	/* Parse the 1-D or 2-D FFT options such as -N in grdfft */
	unsigned int n_errors = 0, pos = 0;
	char p[GMT_BUFSIZ] = {""}, *c = NULL;
	struct GMT_FFT_INFO *info = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	if (args == NULL) return_null (V_API, GMT_ARG_IS_NULL);
	if (dim == 0) return_null (V_API, GMT_DIM_TOO_SMALL);
	if (dim > 2) return_null (V_API, GMT_DIM_TOO_LARGE);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	info = gmt_M_memory (API->GMT, NULL, 1, struct GMT_FFT_INFO);
	info->taper_width = -1.0;				/* Not set yet */
	info->taper_mode = GMT_FFT_EXTEND_NOT_SET;		/* Not set yet */
	info->trend_mode = GMT_FFT_REMOVE_NOT_SET;		/* Not set yet */
	info->info_mode = GMT_FFT_UNSPECIFIED;			/* Not set yet */
	info->suggest = GMT_FFT_N_SUGGEST;				/* Not yet set */

	if ((c = strchr (args, '+'))) {	/* Handle modifiers */
		while ((gmt_strtok (c, "+", &pos, p))) {
			switch (p[0]) {
				/* Detrending modifiers */
				case 'a':  info->trend_mode = GMT_FFT_REMOVE_MEAN;  break;
				case 'd':  info->trend_mode = GMT_FFT_REMOVE_TREND; break;
				case 'h':  info->trend_mode = GMT_FFT_REMOVE_MID;   break;
				case 'l':  info->trend_mode = GMT_FFT_REMOVE_NOTHING;  break;
				/* Taper modifiers */
				case 'e':  info->taper_mode = GMT_FFT_EXTEND_POINT_SYMMETRY; break;
				case 'n':  info->taper_mode = GMT_FFT_EXTEND_NONE; break;
				case 'm':  info->taper_mode = GMT_FFT_EXTEND_MIRROR_SYMMETRY; break;
				case 't':	/* Set taper width */
					if ((info->taper_width = atof (&p[1])) < 0.0) {
						GMT_Report (API, GMT_MSG_NORMAL, "Error -%c: Negative taper width given\n", option);
						n_errors++;
					}
					break;
				/* i/o modifiers */
				case 'w':	/* Save FFT input; optionally append file suffix */
					info->save[GMT_IN] = true;
					if (p[1]) strncpy (info->suffix, &p[1], GMT_LEN64-1);
					break;
				case 'v':  info->verbose = true; break;	/* Report FFT suggestions */
				case 'z': 	/* Save FFT output in two files; append p for polar form */
					info->save[GMT_OUT] = true;
					if (p[1] == 'p') info->polar = true;
					break;
				default:
					GMT_Report (API, GMT_MSG_NORMAL, "Error -%c: Unrecognized modifier +%s.\n", option, p);
					n_errors++;
					break;
			}
		}
	}
	if (info->taper_mode == GMT_FFT_EXTEND_NOT_SET)
		info->taper_mode = GMT_FFT_EXTEND_POINT_SYMMETRY;	/* Default action is edge-point symmetry */
	if (info->taper_mode == GMT_FFT_EXTEND_NONE) {
		if (info->taper_width < 0.0) info->taper_width = 0.0;	/* No tapering unless specified */
	}
	if (info->taper_width < 0.0)
		info->taper_width = 100.0;		/* Taper over entire margin strip by default */

	switch (args[0]) {
		case '\0': info->suggest = GMT_FFT_N_SUGGEST;  break;	/* Pick dimensions for the "best" solution */
		case 'a': info->suggest = GMT_FFT_ACCURATE;  break;	/* Pick dimensions for most accurate solution */
		case 'f': info->info_mode = GMT_FFT_FORCE; break;	/* Default is force actual grid dimensions */
		case 'm': info->suggest = GMT_FFT_STORAGE;  break;	/* Pick dimensions for minimum storage */
		case 'q': info->verbose = true; break;	/* No longer a mode.  Backwards compatibility; see +v instead */
		case 'r': info->suggest = GMT_FFT_FAST;  break;	/* Pick dimensions for most rapid solution */
		case 's': info->info_mode = GMT_FFT_LIST;  break;
		default:
			if (dim == 2U) {	/* 2-D */
				pos = sscanf (args, "%d/%d", &info->n_columns, &info->n_rows);
				if (pos == 1) info->n_rows = info->n_columns;
			}
			else {	/* 1-D */
				pos = sscanf (args, "%d", &info->n_columns);
				info->n_rows = 0;
			}
			if (pos) info->info_mode = GMT_FFT_SET;
	}
	if (info->suffix[0] == '\0') strncpy (info->suffix, "tapered", GMT_LEN64-1);	/* Default suffix */
	info->set = true;	/* We parsed this option */
	if (info->info_mode == GMT_FFT_SET) {
		if (dim == 2U && (info->n_columns <= 0 || info->n_rows <= 0)) {
			GMT_Report (API, GMT_MSG_NORMAL, "Error -%c: n_columns and/or n_rows are <= 0\n", option);
			n_errors++;
		}
		else if (dim == 1U && info->n_columns <= 0) {
			GMT_Report (API, GMT_MSG_NORMAL, "Error -%c: n_columns is <= 0\n", option);
			n_errors++;
		}
	}
	if (info->taper_mode == GMT_FFT_EXTEND_NONE && info->taper_width == 100.0) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error -%c: +n requires +t with width << 100!\n", option);
		n_errors++;
	}
	if (info->info_mode == GMT_FFT_LIST) {
		fft_Singleton_list (API);
		n_errors++;	/* So parsing fails and stops the program after this listing */
	}
	if (n_errors) {
		gmt_M_free (API->GMT, info);
		info = NULL;
	}
	return (info);
}

#ifdef FORTRAN_API
void *GMT_FFT_Parse_ (char *option, unsigned int *dim, char *args, int *length) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_FFT_Parse (GMT_FORTRAN, *option, *dim, args));
}
#endif

/*! . */
GMT_LOCAL struct GMT_FFT_WAVENUMBER * api_fft_init_1d (struct GMTAPI_CTRL *API, struct GMT_DATASET *D, unsigned int mode, void *v_info) {
	struct GMT_FFT_WAVENUMBER *K = NULL;
	gmt_M_unused(API); gmt_M_unused(D); gmt_M_unused(mode); gmt_M_unused(v_info);

#if 0	/* Have not finalized 1-D FFT usage in general; this will probably happen when we add gmtfft [1-D FFT equivalent to grdfft] */
	unsigned n_cols = 1;
	struct GMT_FFT_INFO *F = api_get_fftinfo_ptr (v_info);
	/* Determine number of columns in [t] x [y] input */
	if (mode & GMT_FFT_CROSS_SPEC) n_cols++;
	if (Din->n_columns < n_cols) {
		GMT_report (API, GMT_MSG_NORMAL, "Error: 2 columns needed but only 1 provided\n");
		return NULL;
	}
	cross = (n_cols == 2);
	if (mode & GMT_FFT_DELTA) n_cols++;
	delta_t = (mode & GMT_FFT_DELTA) ? F->delta_t : D->table[0]->segment[0]->data[0][1] - D->table[0]->segment[0]->data[0][0];
	K->delta_kx = 2.0 * M_PI / (F->n_columns * delta_t);

	GMT_table_detrend (C, D, F->trend_mode, K->coeff);	/* Detrend data, if requested */
	gmt_table_taper (C, G, F);				/* Taper data, if requested */
	K->dim = 1;	/* 1-D FFT */
#endif
	return (K);
}

/*! . */
GMT_LOCAL void fft_taper2d (struct GMT_CTRL *GMT, struct GMT_GRID *Grid, struct GMT_FFT_INFO *F) {
	/* mode sets if and how tapering will be performed [see GMT_FFT_EXTEND_* constants].
	 * width is relative width in percent of the margin that will be tapered [100]. */
	int il1, ir1, il2, ir2, jb1, jb2, jt1, jt2, im, jm, j, end_i, end_j, min_i, min_j, one;
	int i, i_data_start, j_data_start, mx, i_width, j_width, width_percent;
	unsigned int ju, start_component = 0, stop_component = 0, component;
	uint64_t off;
	char *method[2] = {"edge-point", "mirror"}, *comp[2] = {"real", "imaginary"};
	float *datac = Grid->data, scale, cos_wt;
	double width;
	struct GMT_GRID_HEADER *h = Grid->header;	/* For shorthand */

	width_percent = irint (F->taper_width);

	if ((Grid->header->n_columns == F->n_columns && Grid->header->n_rows == F->n_rows) || F->taper_mode == GMT_FFT_EXTEND_NONE) {
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Data and FFT dimensions are equal - no data extension will take place\n");
		/* But there may still be interior tapering */
		if (F->taper_mode != GMT_FFT_EXTEND_NONE) {	/* Nothing to do since no outside pad */
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Data and FFT dimensions are equal - no tapering will be performed\n");
			return;
		}
		if (F->taper_mode == GMT_FFT_EXTEND_NONE && width_percent == 100) {	/* No interior taper specified */
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "No interior tapering will be performed\n");
			return;
		}
	}

	if (Grid->header->arrangement == GMT_GRID_IS_INTERLEAVED) {
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Demultiplexing complex grid before tapering can take place.\n");
		gmt_grd_mux_demux (GMT, Grid->header, Grid->data, GMT_GRID_IS_SERIAL);
	}

	/* Note that if nx2 = nx+1 and ny2 = n_rows + 1, then this routine
	 * will do nothing; thus a single row/column of zeros may be
	 * added to the bottom/right of the input array and it cannot
	 * be tapered.  But when (nx2 - nx)%2 == 1 or ditto for y,
	 * this is zero anyway.  */

	i_data_start = GMT->current.io.pad[XLO];	/* Some shorthands for readability */
	j_data_start = GMT->current.io.pad[YHI];
	mx = h->mx;
	one = (F->taper_mode == GMT_FFT_EXTEND_NONE) ? 0 : 1;	/* 0 is the boundary point which we want to taper to 0 for the interior taper */

	if (width_percent == 0) {
		GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Tapering has been disabled via +t0\n");
	}
	if (width_percent == 100 && F->taper_mode == GMT_FFT_EXTEND_NONE) {	/* Means user set +n but did not specify +t<taper> as 100% is unreasonable for interior */
		width_percent = 0;
		width = 0.0;
	}
	else
		width = F->taper_width / 100.0;	/* Was percent, now fraction */

	if (F->taper_mode == GMT_FFT_EXTEND_NONE) {	/* No extension, just tapering inside the data grid */
		i_width = irint (Grid->header->n_columns * width);	/* Interior columns over which tapering will take place */
		j_width = irint (Grid->header->n_rows * width);	/* Extended rows over which tapering will take place */
	}
	else {	/* We wish to extend data into the margin pads between FFT grid and data grid */
		i_width = irint (i_data_start * width);	/* Extended columns over which tapering will take place */
		j_width = irint (j_data_start * width);	/* Extended rows over which tapering will take place */
	}
	if (i_width == 0 && j_width == 0) one = 1;	/* So we do nothing further down */

	/* Determine how many complex components (1 or 2) to taper, and which one(s) */
	start_component = (Grid->header->complex_mode & GMT_GRID_IS_COMPLEX_REAL) ? 0 : 1;
	stop_component  = (Grid->header->complex_mode & GMT_GRID_IS_COMPLEX_IMAG) ? 1 : 0;

	for (component = start_component; component <= stop_component; component++) {	/* Loop over 1 or 2 components */
		off = component * Grid->header->size / 2;	/* offset to start of this component in grid */

		/* First reflect about xmin and xmax, either point symmetric about edge point OR mirror symmetric */

		if (F->taper_mode != GMT_FFT_EXTEND_NONE) {
			for (im = 1; im <= i_width; im++) {
				il1 = -im;	/* Outside xmin; left of edge 1  */
				ir1 = im;	/* Inside xmin; right of edge 1  */
				il2 = il1 + h->n_columns - 1;	/* Inside xmax; left of edge 2  */
				ir2 = ir1 + h->n_columns - 1;	/* Outside xmax; right of edge 2  */
				for (ju = 0; ju < h->n_rows; ju++) {
					if (F->taper_mode == GMT_FFT_EXTEND_POINT_SYMMETRY) {
						datac[gmt_M_ijp(h,ju,il1)+off] = 2.0f * datac[gmt_M_ijp(h,ju,0)+off]       - datac[gmt_M_ijp(h,ju,ir1)+off];
						datac[gmt_M_ijp(h,ju,ir2)+off] = 2.0f * datac[gmt_M_ijp(h,ju,h->n_columns-1)+off] - datac[gmt_M_ijp(h,ju,il2)+off];
					}
					else {	/* Mirroring */
						datac[gmt_M_ijp(h,ju,il1)+off] = datac[gmt_M_ijp(h,ju,ir1)+off];
						datac[gmt_M_ijp(h,ju,ir2)+off] = datac[gmt_M_ijp(h,ju,il2)+off];
					}
				}
			}
		}

		/* Next, reflect about ymin and ymax.
		 * At the same time, since x has been reflected,
		 * we can use these vals and taper on y edges */

		scale = (float)(M_PI / (j_width + 1));	/* Full 2*pi over y taper range */
		min_i = (F->taper_mode == GMT_FFT_EXTEND_NONE) ? 0 : -i_width;
		end_i = (F->taper_mode == GMT_FFT_EXTEND_NONE) ? (int)Grid->header->n_columns : mx - i_width;
		for (jm = one; jm <= j_width; jm++) {	/* Loop over width of strip to taper */
			jb1 = -jm;	/* Outside ymin; bottom side of edge 1  */
			jt1 = jm;	/* Inside ymin; top side of edge 1  */
			jb2 = jb1 + h->n_rows - 1;	/* Inside ymax; bottom side of edge 2  */
			jt2 = jt1 + h->n_rows - 1;	/* Outside ymax; bottom side of edge 2  */
			cos_wt = 0.5f * (1.0f + cosf (jm * scale));
			if (F->taper_mode == GMT_FFT_EXTEND_NONE) cos_wt = 1.0f - cos_wt;	/* Reverse weights for the interior */
			for (i = min_i; i < end_i; i++) {
				if (F->taper_mode == GMT_FFT_EXTEND_POINT_SYMMETRY) {
					datac[gmt_M_ijp(h,jb1,i)+off] = cos_wt * (2.0f * datac[gmt_M_ijp(h,0,i)+off]       - datac[gmt_M_ijp(h,jt1,i)+off]);
					datac[gmt_M_ijp(h,jt2,i)+off] = cos_wt * (2.0f * datac[gmt_M_ijp(h,h->n_rows-1,i)+off] - datac[gmt_M_ijp(h,jb2,i)+off]);
				}
				else if (F->taper_mode == GMT_FFT_EXTEND_MIRROR_SYMMETRY) {
					datac[gmt_M_ijp(h,jb1,i)+off] = cos_wt * datac[gmt_M_ijp(h,jt1,i)+off];
					datac[gmt_M_ijp(h,jt2,i)+off] = cos_wt * datac[gmt_M_ijp(h,jb2,i)+off];
				}
				else {	/* Interior tapering only */
					datac[gmt_M_ijp(h,jt1,i)+off] *= cos_wt;
					datac[gmt_M_ijp(h,jb2,i)+off] *= cos_wt;
				}
			}
		}
		/* Now, cos taper the x edges */
		scale = (float)(M_PI / (i_width + 1));	/* Full 2*pi over x taper range */
		end_j = (F->taper_mode == GMT_FFT_EXTEND_NONE) ? h->n_rows : h->my - j_data_start;
		min_j = (F->taper_mode == GMT_FFT_EXTEND_NONE) ? 0 : -j_width;
		for (im = one; im <= i_width; im++) {
			il1 = -im;
			ir1 = im;
			il2 = il1 + h->n_columns - 1;
			ir2 = ir1 + h->n_columns - 1;
			cos_wt = (float)(0.5f * (1.0f + cosf (im * scale)));
			if (F->taper_mode == GMT_FFT_EXTEND_NONE) cos_wt = 1.0f - cos_wt;	/* Switch to weights for the interior */
			for (j = min_j; j < end_j; j++) {
				if (F->taper_mode == GMT_FFT_EXTEND_NONE) {
					datac[gmt_M_ijp(h,j,ir1)+off] *= cos_wt;
					datac[gmt_M_ijp(h,j,il2)+off] *= cos_wt;
				}
				else {
					datac[gmt_M_ijp(h,j,il1)+off] *= cos_wt;
					datac[gmt_M_ijp(h,j,ir2)+off] *= cos_wt;
				}
			}
		}

		if (F->taper_mode == GMT_FFT_EXTEND_NONE)
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Grid margin (%s component) tapered to zero over %d %% of data width and height\n", comp[component], width_percent);
		else
			GMT_Report (GMT->parent, GMT_MSG_LONG_VERBOSE, "Grid (%s component) extended via %s symmetry at all edges, then tapered to zero over %d %% of extended area\n", comp[component], method[F->taper_mode], width_percent);
	}
}

/*! . */
GMT_LOCAL struct GMT_FFT_WAVENUMBER *api_fft_init_2d (struct GMTAPI_CTRL *API, struct GMT_GRID *G, unsigned int mode, void *v_info) {
	/* Initialize grid dimensions for FFT machinery and set up wavenumbers */
	unsigned int k, factors[32];
	uint64_t node;
	size_t worksize;
	bool stop;
	double tdummy, edummy;
	struct GMT_FFT_SUGGESTION fft_sug[GMT_FFT_N_SUGGEST];
	struct GMT_FFT_INFO *F = NULL, *F_in = api_get_fftinfo_ptr (v_info);
	struct GMT_FFT_WAVENUMBER *K = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	if (G == NULL) return_null (API, GMT_ARG_IS_NULL);
	GMT = API->GMT;
	K = gmt_M_memory (GMT, NULL, 1, struct GMT_FFT_WAVENUMBER);

	F = gmt_M_memory (GMT, NULL, 1, struct GMT_FFT_INFO);
	if (F_in) {	/* User specified -N so default settings should take effect */
		gmt_M_memcpy (F, F_in, 1, struct GMT_FFT_INFO);
		if (F->K) GMT_Report (API, GMT_MSG_DEBUG, "Warning: F->K already set; investigate.\n");
	}
	if (!F->set || F->info_mode == GMT_FFT_UNSPECIFIED) {	/* User is accepting the default values of extend via edge-point symmetry over 100% of margin */
		F->info_mode = GMT_FFT_EXTEND_POINT_SYMMETRY;
		F->taper_width = 100.0;
		if (!F->set) F->suggest = GMT_FFT_N_SUGGEST;
		F->set = true;
	}

	/* Get dimensions as may be appropriate */
	if (F->info_mode == GMT_FFT_SET) {	/* User specified the n_columns/n_rows dimensions */
		if (F->n_columns < G->header->n_columns || F->n_rows < G->header->n_rows) {
			GMT_Report (API, GMT_MSG_NORMAL, "Warning: You specified a FFT n_columns/n_rows smaller than input grid.  Ignored.\n");
			F->info_mode = GMT_FFT_EXTEND;
		}
	}

	if (F->info_mode != GMT_FFT_SET) {	/* Either adjust, force, inquiery */
		if (F->info_mode == GMT_FFT_FORCE) {
			F->n_columns = G->header->n_columns;
			F->n_rows = G->header->n_rows;
			GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Selected FFT dimensions == Grid dimensions.\n");
		}
		else {	/* Determine best FFT dimensions */
			unsigned int pick;
			char *mode[GMT_FFT_N_SUGGEST] = {"fastest", "most accurate", "least storage"};
			gmtlib_suggest_fft_dim (GMT, G->header->n_columns, G->header->n_rows, fft_sug, (gmt_M_is_verbose (GMT, GMT_MSG_VERBOSE) || F->verbose));
			if (F->suggest == GMT_FFT_N_SUGGEST) {	/* Must choose smallest of accurate and fast */
				pick = (fft_sug[GMT_FFT_ACCURATE].totalbytes < fft_sug[GMT_FFT_FAST].totalbytes) ? GMT_FFT_ACCURATE : GMT_FFT_FAST;
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Selected FFT dimensions for the overall best solution (%s).\n", mode[pick]);
			}
			else {	/* Pick the one we selected up front */
				pick = F->suggest;
				GMT_Report (API, GMT_MSG_LONG_VERBOSE, "Selected FFT dimensions for the %s solution.\n", mode[pick]);
			}
			F->n_columns = fft_sug[pick].n_columns;
			F->n_rows    = fft_sug[pick].n_rows;
		}
	}

	/* Because we taper and reflect below we DO NOT want any BCs set since that code expects 2 BC rows/cols */
	for (k = 0; k < 4; k++) G->header->BC[k] = GMT_BC_IS_DATA;

	/* Get here when F->n_columns and F->n_rows are set to the values we will use.  */

	gmtfft_fourt_stats (GMT, F->n_columns, F->n_rows, factors, &edummy, &worksize, &tdummy);
	GMT_Report (API, GMT_MSG_VERBOSE, "Grid dimensions (n_rows by n_columns): %d x %d\tFFT dimensions: %d x %d\n", G->header->n_rows, G->header->n_columns, F->n_rows, F->n_columns);

	/* Put the data in the middle of the padded array */

	GMT->current.io.pad[XLO] = (F->n_columns - G->header->n_columns) / 2;	/* zero if n_columns < G->header->n_columns+1  */
	GMT->current.io.pad[YHI] = (F->n_rows - G->header->n_rows) / 2;
	GMT->current.io.pad[XHI] = F->n_columns - G->header->n_columns - GMT->current.io.pad[XLO];
	GMT->current.io.pad[YLO] = F->n_rows - G->header->n_rows - GMT->current.io.pad[YHI];

	/* Precompute wavenumber increments and initialize the GMT_FFT machinery */

	K->delta_kx = 2.0 * M_PI / (F->n_columns * G->header->inc[GMT_X]);
	K->delta_ky = 2.0 * M_PI / (F->n_rows * G->header->inc[GMT_Y]);
	K->nx2 = F->n_columns;	K->ny2 = F->n_rows;

	if (gmt_M_is_geographic (GMT, GMT_IN)) {	/* Give delta_kx, delta_ky units of 2pi/meters via Flat Earth assumtion  */
		K->delta_kx /= (GMT->current.proj.DIST_M_PR_DEG * cosd (0.5 * (G->header->wesn[YLO] + G->header->wesn[YHI])));
		K->delta_ky /= GMT->current.proj.DIST_M_PR_DEG;
	}

	gmt_fft_set_wave (GMT, GMT_FFT_K_IS_KR, K);	/* Initialize for use with radial wavenumbers */

	F->K = K;	/* So that F can access information in K later */
	K->info = F;	/* So K can have access to information in F later */

	/* Read in the data or change pad to match the nx2/ny2 determined */

	if (G->data) {	/* User already read the data, check padding and possibly extend it */
		if (G->header->complex_mode == 0) {	/* Grid was not read in interleaved, must do so now */
			/* Because of no realloc for aligned memory we must do it the hard way */
			float *f = NULL;
			size_t new_size = 2*G->header->size;
			GMT_Report (API, GMT_MSG_VERBOSE, "Must double memory and multiplex external grid before we can do FFT!\n", G->header->name);
			GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Extend grid via copy onto larger memory-aligned grid\n");
			f = gmt_M_memory_aligned (GMT, NULL, new_size, float);	/* New, larger grid size */
			gmt_M_memcpy (f, G->data, G->header->size, float);	/* Copy over previous grid values */
			gmt_M_free_aligned (GMT, G->data);			/* Free previous aligned grid memory */
			G->data = f;						/* Attach the new, larger aligned memory */
			G->header->complex_mode = GMT_GRID_IS_COMPLEX_REAL;	/* Flag as complex grid with real components only */
			G->header->size = new_size;				/* Update the size of complex grid */
		}
		if (!(G->header->mx == F->n_columns && G->header->my == F->n_rows)) {	/* Must re-pad, possibly re-allocate the grid */
			gmt_grd_pad_on (GMT, G, GMT->current.io.pad);
		}
	}
	else {	/* Read the data into a grid of approved dimension */
		G->header->mx = G->header->n_columns;	G->header->my = G->header->n_rows;	/* Undo misleading padding since we have not read the data yet and GMT pad has changed above */
		if (GMT_Read_Data (GMT->parent, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_DATA_ONLY | mode, NULL, G->header->name, G) == NULL)	/* Get data only */
			return (NULL);
	}
#ifdef DEBUG
	gmt_grd_dump (G->header, G->data, mode & GMT_GRID_IS_COMPLEX_MASK, "Read in FFT_Create");
#endif
	/* Make sure there are no NaNs in the grid - that is a fatal flaw */

	for (node = 0, stop = false; !stop && node < G->header->size; node++) stop = gmt_M_is_fnan (G->data[node]);
	if (stop) {
		GMT_Report (API, GMT_MSG_NORMAL, "Input grid %s contain NaNs, cannot do FFT!\n", G->header->name);
		return (NULL);
	}

	if (F->trend_mode == GMT_FFT_REMOVE_NOT_SET) F->trend_mode = GMT_FFT_REMOVE_NOTHING;	/* Delayed default */
	gmt_grd_detrend (GMT, G, F->trend_mode, K->coeff);	/* Detrend data, if requested */
#ifdef DEBUG
	gmt_grd_dump (G->header, G->data, mode & GMT_GRID_IS_COMPLEX_MASK, "After detrend");
#endif
	fft_taper2d (GMT, G, F);				/* Taper data, if requested */
#ifdef DEBUG
	gmt_grd_dump (G->header, G->data, mode & GMT_GRID_IS_COMPLEX_MASK, "After Taper");
#endif
	K->dim = 2;	/* 2-D FFT */
	return (K);
}

/*! . */
void *GMT_FFT_Create (void *V_API, void *X, unsigned int dim, unsigned int mode, void *v_info) {
	/* Initialize 1-D or 2-D FFT machinery and set up wavenumbers */
	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	if (dim == 1) return (api_fft_init_1d (V_API, X, mode, v_info));
	if (dim == 2) return (api_fft_init_2d (V_API, X, mode, v_info));
	GMT_Report (V_API, GMT_MSG_NORMAL, "GMT FFT only supports dimensions 1 and 2, not %u\n", dim);
	return_null (V_API, (dim == 0) ? GMT_DIM_TOO_SMALL : GMT_DIM_TOO_LARGE);
}

#ifdef FORTRAN_API
void *GMT_FFT_Create_ (void *X, unsigned int *dim, unsigned int *mode, void *v_info) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_FFT_Create (GMT_FORTRAN, X, *dim, *mode, v_info));
}
#endif

/*! . */
GMT_LOCAL int api_fft_1d (struct GMTAPI_CTRL *API, struct GMT_DATASET *D, int direction, unsigned int mode, struct GMT_FFT_WAVENUMBER *K) {
	/* The 1-D FFT operating on DATASET segments */
	int status = 0;
	uint64_t seg, row, tbl, last = 0, col = 0;
	float *data = NULL;
	struct GMT_DATASEGMENT *S = NULL;
	gmt_M_unused(K);
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	/* Not at all finished; will require gmtfft.c to be developed and tested */
	for (tbl = 0; tbl < D->n_tables; tbl++) {
		for (seg = 0; seg < D->table[tbl]->n_segments; seg++) {
			S = D->table[tbl]->segment[seg];
			if (S->n_rows > last) {	/* Extend array */
				data = gmt_M_memory (API->GMT, data, S->n_rows, float);
				last = S->n_rows;
			}
			for (row = 0; S->n_rows; row++) data[row] = (float)S->data[col][row];
			status = GMT_FFT_1D (API, data, S->n_rows, direction, mode);
			for (row = 0; S->n_rows; row++) S->data[col][row] = data[row];
		}
	}
	gmt_M_free (API->GMT, data);
	return (status);
}

GMT_LOCAL char *fft_file_name_with_suffix (struct GMT_CTRL *GMT, char *name, char *suffix) {
	static char file[GMT_LEN256] = {""};
	uint64_t i, j;
	size_t len;

	if ((len = strlen (name)) == 0) {	/* Grids that are being created have no filename yet */
		snprintf (file, GMT_LEN256, "tmpgrid_%s.grd", suffix);
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Created grid has no name to derive new names from; choose %s\n", file);
		return (file);
	}
	for (i = len; i > 0 && !(name[i] == '/' || name[i] == '\\'); i--);	/* i points to 1st char in name after slash, or 0 if no leading dirs */
	if (i) i++;	/* Move to 1st char after / */
	for (j = len; j > 0 && name[j] != '.'; j--);	/* j points to period before extension, or it is 0 if no extension */
	len = strlen (&name[i]);
	strncpy (file, &name[i], GMT_LEN256-1);		/* Make a full copy of filename without leading directories */
	for (i = len; i > 0 && file[i] != '.'; i--);	/* i now points to period before extension in file, or it is 0 if no extension */
	if (i) file[i] = '\0';	/* Truncate at the extension */
	/* Determine length of new filename and make sure it fits */
	len = strlen (file);
	if (j) len += strlen (&name[j]);
	len += (1 + strlen(suffix));
	if ((GMT_BUFSIZ - len) > 0) {	/* Have enough space */
		strcat (file, "_");
		strcat (file, suffix);
		if (j) strcat (file, &name[j]);
#if 0
		strcat (file, "=bf"); /* Make native for debugging */
#endif
	}
	else
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "File name [ %s] way too long - trouble in fft_file_name_with_suffix\n", file);
	return (file);
}

GMT_LOCAL void fft_grd_save_taper (struct GMT_CTRL *GMT, struct GMT_GRID *Grid, char *suffix) {
	/* Write the intermediate grid that will be passed to the FFT to file.
	 * This grid may have been a mean, mid-value, or plane removed, may
	 * have data filled into an extended margin, and may have been taperer.
	 * Normally, the complex grid will be in serial layout, but just in case
	 * we check and add a demux step if required.  The FFT will also check
	 * and multiplex the grid (again) if needed.
	 */
	unsigned int pad[4];
	struct GMT_GRID_HEADER save;
	char *file = NULL;

	if (Grid->header->arrangement == GMT_GRID_IS_INTERLEAVED) {
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Demultiplexing complex grid before saving can take place.\n");
		gmt_grd_mux_demux (GMT, Grid->header, Grid->data, GMT_GRID_IS_SERIAL);
	}
	gmt_M_memcpy (&save, Grid->header, 1U, struct GMT_GRID_HEADER);	/* Save what we have before messing around */
	gmt_M_memcpy (pad, Grid->header->pad, 4U, unsigned int);		/* Save current pad, then set pad to zero */
	/* Extend w/e/s/n to what it would be if the pad was not present */
	Grid->header->wesn[XLO] -= Grid->header->pad[XLO] * Grid->header->inc[GMT_X];
	Grid->header->wesn[XHI] += Grid->header->pad[XHI] * Grid->header->inc[GMT_X];
	Grid->header->wesn[YLO] -= Grid->header->pad[YLO] * Grid->header->inc[GMT_Y];
	Grid->header->wesn[YHI] += Grid->header->pad[YHI] * Grid->header->inc[GMT_Y];
	gmt_M_memset (Grid->header->pad,   4U, unsigned int);	/* Set header pad to {0,0,0,0} */
	gmt_M_memset (GMT->current.io.pad, 4U, unsigned int);	/* set GMT default pad to {0,0,0,0} */
	gmt_set_grddim (GMT, Grid->header);	/* Recompute all dimensions */
	if ((file = fft_file_name_with_suffix (GMT, Grid->header->name, suffix)) == NULL) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unable to get file name for file %s\n", Grid->header->name);
		return;
	}

	if (GMT_Write_Data (GMT->parent, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_DATA_ONLY | GMT_GRID_IS_COMPLEX_REAL, NULL, file, Grid) != GMT_OK)
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Intermediate detrended, extended, and tapered grid could not be written to %s\n", file);
	else
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Intermediate detrended, extended, and tapered grid written to %s\n", file);

	gmt_M_memcpy (Grid->header, &save, 1U, struct GMT_GRID_HEADER);	/* Restore original, including the original pad */
	gmt_M_memcpy (GMT->current.io.pad, pad, 4U, unsigned int);	/* Restore GMT default pad */
}

GMT_LOCAL void fft_grd_save_fft (struct GMT_CTRL *GMT, struct GMT_GRID *G, struct GMT_FFT_INFO *F) {
	/* Save the raw spectrum as two files (real,imag) or (mag,phase), depending on mode.
	 * We must first do an "fftshift" operation as in Matlab, to put the 0 frequency
	 * value in the center of the grid. */
	uint64_t i_ij, o_ij,  offset;
	int row_in, col_in, row_out, col_out, nx_2, ny_2;
	size_t len;
	unsigned int k, pad[4], mode, wmode[2] = {GMT_GRID_IS_COMPLEX_REAL, GMT_GRID_IS_COMPLEX_IMAG};
	double wesn[4], inc[2];
	float re, im, i_scale;
	char *file = NULL, *suffix[2][2] = {{"real", "imag"}, {"mag", "phase"}};
	struct GMT_GRID *Out = NULL;
	struct GMT_FFT_WAVENUMBER *K = F->K;

	if (K == NULL) return;

	mode = (F->polar) ? 1 : 0;

	GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Write components of complex raw spectrum with file suffix %s and %s\n", suffix[mode][0], suffix[mode][1]);

	if (G->header->arrangement == GMT_GRID_IS_SERIAL) {
		GMT_Report (GMT->parent, GMT_MSG_VERBOSE, "Cannot save complex grid unless it is interleaved.\n");
		return;
	}
	/* Prepare wavenumber domain limits and increments */
	nx_2 = K->nx2 / 2;	ny_2 = K->ny2 / 2;
	wesn[XLO] = -K->delta_kx * nx_2;	wesn[XHI] =  K->delta_kx * (nx_2 - 1);
	wesn[YLO] = -K->delta_ky * (ny_2 - 1);	wesn[YHI] =  K->delta_ky * ny_2;
	inc[GMT_X] = K->delta_kx;		inc[GMT_Y] = K->delta_ky;
	gmt_M_memcpy (pad, GMT->current.io.pad, 4U, unsigned int);	/* Save current GMT pad */
	for (k = 0; k < 4; k++) GMT->current.io.pad[k] = 0;		/* No pad is what we need for this application */

	/* Set up and allocate the temporary grid which is always gridline registered. */
	if ((Out = GMT_Create_Data (GMT->parent, GMT_IS_GRID, GMT_IS_SURFACE, GMT_CONTAINER_AND_DATA | GMT_GRID_IS_COMPLEX_MASK, NULL, wesn, inc, \
		GMT_GRID_NODE_REG, 0, NULL)) == NULL) {	/* Note: 0 for pad since no BC work needed for this temporary grid */
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unable to create complex output grid for %s\n", G->header->name);
		return;
	}

	strcpy (Out->header->x_units, "xunit^(-1)");	strcpy (Out->header->y_units, "yunit^(-1)");
	strcpy (Out->header->z_units, G->header->z_units);
	strcpy (Out->header->remark, "Applied fftshift: kx,ky = (0,0) now at (n_columns/2 + 1,n_rows/2");

	offset = Out->header->size / 2;
	i_scale = (float)(1.0  / Out->header->nm);
	for (row_in = 0; row_in < K->ny2; row_in++) {
		row_out = (row_in + ny_2) % K->ny2;
		for (col_in = 0; col_in < K->nx2; col_in++) {
			col_out = (col_in + nx_2) % K->nx2;
			o_ij = gmt_M_ij0 (Out->header, row_out, col_out);
			i_ij = 2 * gmt_M_ij0 (G->header,   row_in,  col_in);
			re = G->data[i_ij] * i_scale; im = G->data[i_ij+1] * i_scale;
			if (F->polar) {	/* Want magnitude and phase */
				Out->data[o_ij]   = (float)hypot (re, im);
				Out->data[o_ij+offset] = (float)d_atan2 (im, re);
			}
			else {		/* Retain real and imag components as is */
				Out->data[o_ij] = re;	Out->data[o_ij+offset] = im;
			}
		}
	}
	for (k = 0; k < 2; k++) {	/* Write the two grids */
		if ((file = fft_file_name_with_suffix (GMT, G->header->name, suffix[mode][k])) == NULL) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Unable to get file name for file %s\n", G->header->name);
			return;
		}
		Out->header->complex_mode = wmode[k];
		for (len = strlen (G->header->name); len > 0 && !(G->header->name[len-1] == '/' || G->header->name[len-1] == '\\'); len--);	/* Find start of file name minus any leading directories */
		snprintf (Out->header->title, GMT_GRID_TITLE_LEN80, "The %s part of FFT transformed grid %s", suffix[mode][k], &G->header->name[len]);
		if (k == 1 && mode) strcpy (Out->header->z_units, "radians");
		if (GMT_Write_Data (GMT->parent, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_CONTAINER_AND_DATA | wmode[k], NULL, file, Out) != GMT_OK) {
			GMT_Report (GMT->parent, GMT_MSG_NORMAL, "%s could not be written\n", file);
			return;
		}
	}
	if (GMT_Destroy_Data (GMT->parent, &Out) != GMT_OK) {
		GMT_Report (GMT->parent, GMT_MSG_NORMAL, "Error freeing temporary grid\n");
	}

	gmt_M_memcpy (GMT->current.io.pad, pad, 4U, unsigned int);	/* Restore GMT pad */
}

GMT_LOCAL void fft_save2d (struct GMT_CTRL *GMT, struct GMT_GRID *G, unsigned int direction, struct GMT_FFT_WAVENUMBER *K) {
	/* Handle the writing of the grid going into the FFT and comping out of the FFT, per F settings */

	if (G == NULL || (K == NULL ||  K->info == NULL)) return;
	if (direction == GMT_IN  && K->info->save[GMT_IN])  fft_grd_save_taper (GMT, G, K->info->suffix);
	if (direction == GMT_OUT && K->info->save[GMT_OUT]) fft_grd_save_fft (GMT, G, K->info);
}

/*! . */
GMT_LOCAL int api_fft_2d (struct GMTAPI_CTRL *API, struct GMT_GRID *G, int direction, unsigned int mode, struct GMT_FFT_WAVENUMBER *K) {
	/* The 2-D FFT operating on GMT_GRID arrays */
	int status;
	if (K && direction == GMT_FFT_FWD) fft_save2d (API->GMT, G, GMT_IN, K);	/* Save intermediate grid, if requested, before interleaving */
	gmt_grd_mux_demux (API->GMT, G->header, G->data, GMT_GRID_IS_INTERLEAVED);
#ifdef DEBUG
	gmt_grd_dump (G->header, G->data, true, "After demux");
#endif
	status = GMT_FFT_2D (API, G->data, G->header->mx, G->header->my, direction, mode);
#ifdef DEBUG
	gmt_grd_dump (G->header, G->data, true, "After FFT");
#endif
	if (K && direction == GMT_FFT_FWD) fft_save2d (API->GMT, G, GMT_OUT, K);	/* Save complex grid, if requested */
	return (status);
}

/*! . */
int GMT_FFT (void *V_API, void *X, int direction, unsigned int mode, void *v_K) {
	/* The 1-D or 2-D FFT operating on GMT_DATASET or GMT_GRID arrays */
	struct GMT_FFT_WAVENUMBER *K = api_get_fftwave_ptr (v_K);
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (K->dim == 2) return (api_fft_2d (V_API, X, direction, mode, K));
	else return (api_fft_1d (V_API, X, direction, mode, K));
}

#ifdef FORTRAN_API
int GMT_FFT_ (void *X, int *direction, unsigned int *mode, void *v_K) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_FFT (GMT_FORTRAN, X, *direction, *mode, v_K));
}
#endif

/*! . */
int GMT_FFT_Destroy (void *V_API, void *v_info) {
	/* Perform any final duties, perhaps report.  For now just free */
	struct GMT_FFT_WAVENUMBER **K = NULL;
	struct GMTAPI_CTRL *API = NULL;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	K = api_get_fftwave_addr (v_info);
	gmt_M_free (API->GMT, (*K)->info);
	gmt_M_free (API->GMT, (*K));
	return_error (V_API, GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_FFT_Destroy_ (void *v_K) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_FFT_Destroy (GMT_FORTRAN, v_K));
}
#endif

/*! Pretty print core module names and purposes */
void gmt_show_name_and_purpose (void *API, const char *component, const char *name, const char *purpose) {
	char message[GMT_LEN256] = {""};
	const char *lib = NULL;
	static char *core = "core";
	assert (name != NULL);
	assert (purpose != NULL);
	lib = (component) ? component : core;
	snprintf (message, GMT_LEN256, "%s(%s) %s - %s\n\n", name, lib, GMT_version(), purpose);
	GMT_Message (API, GMT_TIME_NONE, message);
}

/* Module Extension: Allow listing and calling modules by name */

/*! . */
GMT_LOCAL void *api_get_shared_module_func (struct GMTAPI_CTRL *API, const char *module, unsigned int lib_no) {
	/* Function that returns a pointer to the function named module in specified shared library lib_no, or NULL if not found  */
	void *p_func = NULL;       /* function pointer */
	if (API->lib[lib_no].skip) return (NULL);	/* Tried to open this shared library before and it was not available */
	if (API->lib[lib_no].handle == NULL && (API->lib[lib_no].handle = dlopen (API->lib[lib_no].path, RTLD_LAZY)) == NULL) {	/* Not opened this shared library yet */
		GMT_Report (API, GMT_MSG_NORMAL, "Unable to open GMT shared %s library: %s\n", API->lib[lib_no].name, dlerror());
		API->lib[lib_no].skip = true;	/* Not bother the next time... */
		return (NULL);			/* ...and obviously no function would be found */
	}
	/* Here the library handle is available; try to get pointer to specified module */
	*(void **) (&p_func) = dlsym (API->lib[lib_no].handle, module);
	return (p_func);
}

#ifndef BUILD_SHARED_LIBS
EXTERN_MSC void *gmt_core_module_lookup (struct GMTAPI_CTRL *API, const char *candidate);
#endif

/*! . */
GMT_LOCAL void *api_get_module_func (struct GMTAPI_CTRL *API, const char *module, unsigned int lib_no) {
#ifndef BUILD_SHARED_LIBS
	if (lib_no == 0)	/* Get core module */
		return (gmt_core_module_lookup (API, module));
	/* Else we get custom module below */
#endif
	return (api_get_shared_module_func (API, module, lib_no));
}

/*! . */
int GMT_Call_Module (void *V_API, const char *module, int mode, void *args) {
	/* Call the specified shared module and pass it the mode and args.
 	 * mode can be one of the following:
	 * GMT_MODULE_LIST [-4]:	As GMT_MODULE_PURPOSE, but only lists the modules.
	 * GMT_MODULE_EXIST [-3]:	Return GMT_NOERROR (0) if module exists, GMT_NOT_A_VALID_MODULE otherwise.
	 * GMT_MODULE_PURPOSE [-2]:	As GMT_MODULE_EXIST, but also print the module purpose.
	 * GMT_MODULE_OPT [-1]:		Args is a linked list of option structures.
	 * GMT_MODULE_CMD [0]:		Args is a single textstring with multiple options
	 * mode > 0:				Args is an array of text strings (e.g., argv[]).
	 */
	int status = GMT_NOERROR;
	unsigned int lib;
	struct GMTAPI_CTRL *API = NULL;
	char gmt_module[GMT_LEN32] = "GMT_";
	int (*p_func)(void*, int, void*) = NULL;       /* function pointer */

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (module == NULL && !(mode == GMT_MODULE_LIST || mode == GMT_MODULE_PURPOSE))
		return_error (V_API, GMT_ARG_IS_NULL);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	if (module == NULL) {	/* Did not specify any specific module, so list purpose of all modules in all shared libs */
		char gmt_module[GMT_LEN256] = {""};	/* To form name of gmt_<lib>_module_show|list_all function */
		char *listfunc = (mode == GMT_MODULE_LIST) ? "list" : "show";
		void (*l_func)(void*);       /* function pointer to gmt_<lib>_module_show|list_all which takes one arg (the API) */

		/* Here we list purpose of all the available modules in each shared library */
		for (lib = 0; lib < API->n_shared_libs; lib++) {
			snprintf (gmt_module, GMT_LEN32, "gmt_%s_module_%s_all", API->lib[lib].name, listfunc);
			*(void **) (&l_func) = api_get_module_func (API, gmt_module, lib);
			if (l_func == NULL) continue;	/* Not found in this shared library */
			(*l_func) (V_API);	/* Run this function */
		}
		return (status);
	}

	/* Here we call a named module */

	strncat (gmt_module, module, GMT_LEN32-5);		/* Concatenate GMT_-prefix and module name to get function name */
	for (lib = 0; lib < API->n_shared_libs; lib++) {	/* Look for gmt_module in any of the shared libs */
		*(void **) (&p_func) = api_get_module_func (API, gmt_module, lib);
		if (p_func) break;	/* Found it in this shared library */
	}
	if (p_func == NULL) {	/* Not in any of the shared libraries */
		status = GMT_NOT_A_VALID_MODULE;
		if (strncasecmp (module, "gmt", 3)) {	/* For any module not already starting with "gmt..." */
			char gmt_module[32] = "gmt";
			strncat (gmt_module, module, 28);	/* Prepend "gmt" to module and try again */
			status = GMT_Call_Module (V_API, gmt_module, mode, args);	/* Recursive call to try with the 'gmt' prefix */
		}
	}
	else if (mode == GMT_MODULE_EXIST)	/* Just wanted to know it is there */
		return (GMT_NOERROR);
	else	/* Call the function and pass back its return value */
		status = (*p_func) (V_API, mode, args);
	return (status);
}

#ifdef FORTRAN_API
int GMT_Call_Module_ (const char *module, int *mode, void *args, int *length) {
	return (GMT_Call_Module (GMT_FORTRAN, module, *mode, args));
}
#endif

/*! . */
GMT_LOCAL const char * gmt_get_shared_module_info (struct GMTAPI_CTRL *API, char *module, unsigned int lib_no) {
	/* Function that returns a pointer to the module keys in specified shared library lib_no, or NULL if not found  */
	/* DO not rename this function */
	char function[GMT_LEN64] = {""};
	const char *keys = NULL;       /* char pointer to module keys */
	const char * (*func)(void*, char*) = NULL;       /* function pointer */
	if (API->lib[lib_no].skip) return (NULL);	/* Tried to open this shared library before and it was not available */
	if (API->lib[lib_no].handle == NULL && (API->lib[lib_no].handle = dlopen (API->lib[lib_no].path, RTLD_LAZY)) == NULL) {	/* Not opened this shared library yet */
		GMT_Report (API, GMT_MSG_NORMAL, "Unable to open GMT shared %s library: %s\n", API->lib[lib_no].name, dlerror());
		API->lib[lib_no].skip = true;	/* Not bother the next time... */
		return (NULL);			/* ...and obviously no keys would be found */
	}
	snprintf (function, GMT_LEN64, "gmt_%s_module_info", API->lib[lib_no].name);
	/* Here the library handle is available; try to get pointer to specified module */
	*(void **) (&func) = dlsym (API->lib[lib_no].handle, function);
	if (func) keys = (*func) (API, module);
	return (keys);
}

/*! . */
GMT_LOCAL const char * gmt_get_module_info (struct GMTAPI_CTRL *API, char *module, unsigned int lib_no) {
	/* DO not rename this function */
	if (lib_no == 0)	/* Get core module */
		return (gmt_core_module_info (API, module));
	/* Else we get custom module below */
	return (gmt_get_shared_module_info (API, module, lib_no));
}

/*! . */
GMT_LOCAL const char * api_get_moduleinfo (void *V_API, char *module) {
	/* Call the specified shared module and retrieve the API developer options keys.
 	 * This function, while in the API, is only for API developers and thus has a
	 * "undocumented" status in the API documentation.
	 */
	unsigned int lib;
	struct GMTAPI_CTRL *API = NULL;
	char gmt_module[GMT_LEN32] = "gmt";
	const char *keys = NULL;

	if (V_API == NULL) return_null (NULL, GMT_NOT_A_SESSION);
	if (module == NULL) return_null (V_API, GMT_ARG_IS_NULL);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	for (lib = 0; lib < API->n_shared_libs; lib++) {	/* Look for module in any of the shared libs */
		keys = gmt_get_module_info (API, module, lib);
		if (keys) return (keys);	/* Found it in this shared library, return the keys */
	}
	/* If we get here we did not found it.  Try to prefix module with gmt */
	strncat (gmt_module, module, GMT_LEN32-4);		/* Concatenate gmt and module name to get function name */
	for (lib = 0; lib < API->n_shared_libs; lib++) {	/* Look for gmt_module in any of the shared libs */
		keys = gmt_get_module_info (API, gmt_module, lib);
		if (keys) {	/* Found it in this shared library, adjust module name and return the keys */
			strncpy(module, gmt_module, strlen(gmt_module));	/* Rewrite module name to contain prefix of gmt */
			return (keys);
		}
	}
	/* Not in any of the shared libraries */
	GMT_Report (API, GMT_MSG_VERBOSE, "Shared GMT module not found: %s \n", module);
	return_null (V_API, GMT_NOT_A_VALID_MODULE);
}

GMT_LOCAL int api_extract_argument (char *optarg, char *argument, char **key, int k, bool colon, int *n_pre) {
	/* Two separate actions:
	 * 1) If key ends with "=" then we pull out the option argument after stripping off +<stuff>.
	 * 2) If key ends with "=q" then we see if +q is given and return pos to this modifiers argument.
	 * 3) Else we just copy input to output.
	 * We also set n_pre which is the number of characters to skip after the -X option
	 *   before looking for an argument.
	 * If colon is true we also strip argument from the first colon onwards.
	 * If this all sounds messy it is because the GMT command-line syntax of some modules are
	 *   messy and predate the whole API business...
	 */
	char *c = NULL;
	unsigned int pos = 0;
	*n_pre = 0;
	if (k >= 0 && key[k][K_EQUAL] == '=') {	/* Special handling */
		*n_pre = (key[k][K_MODIFIER] && isdigit (key[k][K_MODIFIER])) ? (int)(key[k][K_MODIFIER]-'0') : 0;
		if ((*n_pre || key[k][K_MODIFIER] == 0) && (c = strchr (optarg, '+'))) {	/* Strip off trailing +<modifiers> */
			c[0] = 0;
			strcpy (argument, optarg);
			c[0] = '+';
		}
		else if (key[k][K_MODIFIER]) {	/* Look for +<mod> */
			char code[3] = {"+?"};
			code[1] = key[k][K_MODIFIER];
			if ((c = strstr (optarg, code))) {	/* Found +<modifier> */
				strcpy (argument, optarg);
				pos = (unsigned int) (c - optarg + 2);	/* Position of this modifiers argument */
			}
			else
				strcpy (argument, optarg);
		}
		else
			strcpy (argument, optarg);
	}
	else
		strcpy (argument, optarg);
	if (colon && (c = strchr (argument, ':')))	/* Also chop of :<more arguments> */
		c[0] = '\0';

	return pos;
}

#define api_is_required_IO(key) (key == API_PRIMARY_INPUT || key == API_PRIMARY_OUTPUT)			/* Returns true if this is a primary input or output item */
#define api_not_required_io(key) ((key == API_PRIMARY_INPUT || key == API_SECONDARY_INPUT) ? API_SECONDARY_INPUT : API_SECONDARY_OUTPUT)	/* Returns the optional input or output flag */

/*! . */
struct GMT_RESOURCE *GMT_Encode_Options (void *V_API, const char *module_name, int n_in, struct GMT_OPTION **head, unsigned int *n) {
	/* This function determines which input sources and output destinations are required given the module options.
	 * It is only used to assist developers of external APIs, such as the MATLAB, Julia, Python, R, and others.
	 * "Keys" referred to below is the unique combination given near the top of every module via the macro
	 * THIS_MODULE_KEYS.  For instance, here are the keys for grdtrack:
	 *
	 * #define THIS_MODULE_KEYS        "<D{,DD),GG(,>D}"
	 *
	 * Here are the GMT_Encode_Options arguments:
	 *   API	Controls all things within GMT.
	 *   module	Name of the GMT module.
	 *   n_in	Known number of objects given as input resources (-1 if not known).
	 *   head	Linked list of GMT options passed for this module. We may hook on 1-2 additional options.
	 *   *n		Number of structures returned by the function. Struct GMT_RESOURCE is defined in gmt.h
	 *
	 * We also return an array of structures with information about registered resources going to/from GMT.
	 * Basically, given the module we look up the keys for that module, which tells us which options provide
	 * the input and output selections and which ones are required and which ones are optional.  We then
	 * scan the given options and if file arguments to the options listed in the keys are missing we are
	 * to insert ? as the filename. Some options may already have the question mark. After scanning
	 * the options we examine the keys for any required input or output argument that have yet to be specified
	 * explicitly. If so we create the missing options, with filename = ?, and append them to the end of
	 * the option list (head). The API developers can then use this array of encoded options in concert with
	 * the information passed back via the structure list to attach actual resources.
	 *
	 * For each option that may take a file we need to know what kind of file and if it is input or output.
	 * We encode this information in a 3-character word XYZ, explained below.  Note that each module may
	 * need several comma-separated XYZ words and these are returned as one string via GMT_Get_Moduleinfo.
	 * The X, Y, and Z letter position represent different information, as discussed below:
	 *
	 * X stands for the specific program OPTION (e.g., L for -L, F for -F). For tables or grids read from files or
	 * tables processed via standard input we use '<', while '>' is used for standard (table) output.
	 * Y stands for data TYPE (C = CPT, D = Dataset/Point, L = Dataset/Line, P = Dataset/Polygon,
	 *    G = Grid, I = Image, T = Textset, X = PostScript, ? = type specified via a module option [more later]),
	 *    while a hyphen (-) means there is NO data when this option is set (see Z for whether this is for in- or output).
	 * Z stands for PRIMARY inputs '{', primary output '}' OR SECONDARY input '(', or secondary output ')'.
	 *   Primary inputs and outputs MUST be assigned, and if not explicitly given will result in
	 *   a syntax error. However, external APIs (mex, Python) can override this and supply the missing items
	 *   via any given left- and right-hand side arguments to supply inputs or accept outputs.
	 *   Secondary inputs means they are only assigned if an option is actually given.  If the in|out designation
	 *   is irrelevant for an option we use '-'.
	 *
	 * There are a few special cases where X, Y, or Z take on "magic" behavior:
	 *
	 *   A few modules with have X = - (hyphen). This means the primary input or output (determined by Z)
	 *   has a data type that is not known until runtime.  A module option will tells us which type it is, and this
	 *   option is encoded in Y.  So a -Y<type> option is _required_ and that is how we can update the primary
	 *   data type.  Example: gmtread can read any GMT object but requires -T<type>.  It thus has the keys
	 *   "<?{,>?},-T-".  Hence, we examine -T<type> and replace ? with the dataset implied by <type> both for input
	 *   AND output (since Z was indeterminate).  Use i|o if only input or output should have this treatment.
	 *
	 *   A few modules will have Y = - which is another magic key: If the -X option is given then either the input
	 *   or output (depending on what Z is) will NOT be required. As an example of this behavior, consider psxy
	 *   which has a -T option that means "read no input, just write trailer". So the key "T-(" in psxy means that
	 *   when -T is used then NO input is required.  This means the primary input key "<D{" is changed to "<D(" (secondary)
	 *   and no attempt is made to connect external input to the psxy input.  If Z is none of () then we expect Z to
	 *   be one of the options with required input (or output) and we change that option to option input (or output).
	 *   Example: grdtrack has two required inputs (the grid(s) and the track/point file.  However, if -E is set then
	 *   the track/point file is not expected so we need to change it to secondary.  We thus add E-< which then will
	 *   change <D{ to <D(.  A modifier is also possible.  For instance -F<grid>[+d] is used by several modules, such
	 *   as triangulate, to use the non-NaN nodes in a grid as the input data instead of reading the primary input
	 *   source.  So F-( would turn of primary input.  However, if +d is present then we want to combine the grid with
	 *   the primary input and hence we read that as well.
	 *
	 *   A few modules will specify Z as some letter not in {|(|}|)|-, which means that normally these modules
	 *   will expect/produce whatever input/output is specified by the primary setting, but if the "-Z" option is given the primary
	 *   input/output will be changed to the given type Y.  Also, modifiers may be involved. The full syntax for this is
	 *   XYZ+abc...-def...: We do the substitution of output type to Y only if
	 *      1. -Z is given
	 *      2. -Z contains ALL the modifiers +a, +b, +c, ...
	 *      3. -Z contains AT LEAST ONE of the modifers +d, +e, +f.
	 *   The Z magic is a bit confusing so here are several examples:
	 *   1. pscoast normally writes PostScript but pscoast -M will instead export data to stdout, so its keys
	 *      contain the entry ">DM", which means that when -M is active then PostScript key ">X}" morphs into ">D}" and
	 *      thus allows for data set export instead.
	 *   2. grdinfo normally writes a textset (key ">T}") but with -C it should write a dataset.  It has the magic
	 *      key ">DC": If the -C option is given then the ">T}" is morphed to ">D}".
	 *   3. grdcontour normally writes PostScript but grdcontour -D will instead export data to a file set by -D, so its key
	 *      contains the entry "DDD": When -D is active then the PostScript key ">X}" morphs into "DD}" and
	 *      thus allows for a data set export instead.
	 *   4. gmtspatial : New key ">TN+r" means if -N[...]+r is given then set >T}.  Just giving -N without the given
	 *      modifier +r will not trigger the change.
	 *   5. pscoast ">TE+w-rR" means if -E given with modifier +w and one of +r or +R are then set to >T}.
	 *
	 *   After processing, all magic key sequences are set to "---" to render them inactive.
	 *
	 */

	unsigned int n_keys, direction = 0, kind, pos = 0, n_items = 0, ku, n_out = 0, nn[2][2];
	unsigned int output_pos = 0, input_pos = 0, mod_pos;
	int family = GMT_NOTSET;	/* -1, or one of GMT_IS_DATASET, GMT_IS_TEXTSET, GMT_IS_GRID, GMT_IS_PALETTE, GMT_IS_IMAGE */
	int geometry = GMT_NOTSET;	/* -1, or one of GMT_IS_NONE, GMT_IS_POINT, GMT_IS_LINE, GMT_IS_POLY, GMT_IS_SURFACE */
	int sdir, k, n_in_added = 0, n_to_add, e, n_pre_arg, n_per_family[GMT_N_FAMILIES];
	bool deactivate_output = false, strip_colon = false, strip = false;
	size_t n_alloc, len;
	const char *keys = NULL;	/* This module's option keys */
	char **key = NULL;		/* Array of items in keys */
	char *text = NULL, *LR[2] = {"rhs", "lhs"}, *S[2] = {" IN", "OUT"}, txt[GMT_LEN256] = {""}, type = 0;
	char *module = NULL, argument[GMT_LEN256] = {""}, strip_colon_opt = 0;
	char *special_text[3] = {" [satisfies required input]", " [satisfies required output]", ""}, *satisfy = NULL;
	struct GMT_OPTION *opt = NULL, *new_ptr = NULL;	/* Pointer to a GMT option structure */
	struct GMT_RESOURCE *info = NULL;	/* Our return array of n_items info structures */
	struct GMTAPI_CTRL *API = NULL;

	*n = 0;	/* Initialize counter to zero in case we return prematurely */

	if (V_API == NULL) return_null (NULL, GMT_NOT_A_SESSION);
	if (module_name == NULL) return_null (V_API, GMT_ARG_IS_NULL);

	if ((*head) && ((*head)->option == GMT_OPT_USAGE || (*head)->option == GMT_OPT_SYNOPSIS)) {	/* Nothing to do */
		*n = UINT_MAX;
		return NULL;
	}
	module = calloc (strlen (module_name) + 4, sizeof(char));	/* Allow space for any "gmt" prefix added to module in api_get_moduleinfo */
	strcpy (module, module_name);			/* This string can grow by 3 if need be */
	/* 0. Get the keys for the module, possibly prepend "gmt" to module if required, or list modules and return NULL if unknown module */
	if ((keys = api_get_moduleinfo (V_API, module)) == NULL) {	/* Gave an unknown module */
		GMT_Call_Module (V_API, NULL, GMT_MODULE_PURPOSE, NULL);	/* List the available modules */
		gmt_M_str_free (module);
		return_null (NULL, GMT_NOT_A_VALID_MODULE);	/* Unknown module code */
	}

	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	/* First some special checks related to unusual GMT syntax or hidden modules */

	/* 1a. Check if this is the write special module, which has flagged its output file as input... */
	if (!strncmp (module, "gmtwrite", 8U) && (opt = GMT_Find_Option (API, GMT_OPT_INFILE, *head))) {
		/* Found a -<"file" option; this is actually the output file so we simply change the option to output */
		opt->option = GMT_OPT_OUTFILE;
		deactivate_output = true;	/* Remember to turn off implicit output option since we got one */
	}
	/* 1b. Check if this is either gmtmath or grdmath which both use the special = outfile syntax and replace that by -=<outfile> */
	else if (!strncmp (module, "gmtmath", 7U) || !strncmp (module, "grdmath", 7U)) {
		struct GMT_OPTION *delete = NULL;
		for (opt = *head; opt && opt->next; opt = opt->next) {	/* Here opt will end up being the last option */
			if (!strcmp (opt->arg, "=")) {
				if (opt->next) {	/* Combine the previous = and <whatever> options into a single -=<whatever> option, then delete the former */
					opt->next->option = '=';
					delete = opt;
				}
			}
		}
		if (delete != *head) GMT_Delete_Option (API, delete);
	}
	/* 1c. Check if this is the grdconvert module, which uses the syntax "infile outfile" without any option flags */
	else if (!strncmp (module, "grdconvert", 10U) && (opt = GMT_Find_Option (API, GMT_OPT_INFILE, *head))) {
		/* Found a -<"file" option; this is indeed the input file but the 2nd "input" is actually output */
		if (opt->next && (opt = GMT_Find_Option (API, GMT_OPT_INFILE, opt->next)))	/* Found the next input file option */
			opt->option = GMT_OPT_OUTFILE;	/* Switch it to an output option */
	}
	/* 1d. Check if this is the greenspline module, where output type is grid for dimension == 2 else it is dataset */
	else if (!strncmp (module, "greenspline", 11U) && (opt = GMT_Find_Option (API, 'R', *head))) {
		/* Found the -R"domain" option; determine the dimensionality of the output */
		unsigned dim = api_determine_dimension (API, opt->arg);
		type = (dim == 2) ? 'G' : 'D';
	}
	/* 1e. Check if this is the triangulate module, where primary dataset output should be turned off if -G given without -M,N,Q,S */
	else if (!strncmp (module, "triangulate", 11U) && (opt = GMT_Find_Option (API, 'G', *head))) {
		/* Found the -G<grid> option; determine if any of -M,N,Q,S are also set */
		if (!((opt = GMT_Find_Option (API, 'M', *head)) || (opt = GMT_Find_Option (API, 'N', *head))
			|| (opt = GMT_Find_Option (API, 'Q', *head)) || (opt = GMT_Find_Option (API, 'S', *head))))
				deactivate_output = true;	/* Turn off implicit output since none is in effect */
	}
	/* 1f. Check if this is the mgd77list module, which writes text or data depending on -F choices */
	else if (!strncmp (module, "mgd77list", 9U) && (opt = GMT_Find_Option (API, 'F', *head))) {
		/* Found the -F option, check if any strings are requested */
		type = 'D';	/* Default is dataset output unless any of the below were requested */
		if (strstr (opt->arg, "all") || strstr (opt->arg, "mgd77") || strstr (opt->arg, "id") || strstr (opt->arg, "sln")
		    || strstr (opt->arg, "sspn") || strstr (opt->arg, "date") || strstr (opt->arg, "recno"))
			type = 'T';
	}
	/* 1g. Check if this is a *contour modules with -Gf|x given. For any other -G? flavor we kill the key with ! */
	else if ((!strncmp (module, "grdcontour", 10U) || !strncmp (module, "pscontour", 9U)) && (opt = GMT_Find_Option (API, 'G', *head))) {
		/* Found the -G option, check if any strings are requested */
		/* If not -Gf|x then we don't want this at all and set type = ! */
		type = (opt->arg[0] == 'f') ? 'T' : ((opt->arg[0] == 'x') ? 'D' : '!');
	}
	/* 1h. Check if this is psxy or psxyz modules with quoted or decorated lines. For any other -S~|q? flavor we kill the key with ! */
	else if ((!strncmp (module, "psxy", 4U) || !strncmp (module, "psxyz", 5U)) && (opt = GMT_Find_Option (API, 'S', *head))) {
		/* Found the -S option, check if we requested quoted or decorated lines via fixed or crossing lines */
		/* If not f|x then we don't want this at all and set type = ! */
		type = (!strchr ("~q", opt->arg[0]) || !strchr ("fx", opt->arg[1])) ? '!' : ((opt->arg[1] == 'x') ? 'D' : 'T');
		strip_colon = (type && strchr (opt->arg, ':'));
		strip_colon_opt = opt->option;
		if (strip_colon)
			GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Got quoted or decorate line and must strip argument %s from colon to end\n", opt->arg);
	}
	/* 1i. Check if this is the talwani3d module, where output type is grid except with -N it is dataset */
	else if (!strncmp (module, "talwani3d", 9U)) {
		/* If we find the -N option, we set type to D, else G */
		type = (GMT_Find_Option (API, 'N', *head)) ? 'D' : 'G';
	}

	gmt_M_str_free (module);

	/* 2a. Get the option key array for this module */
	key = api_process_keys (API, keys, type, *head, n_per_family, &n_keys);	/* This is the array of keys for this module, e.g., "<D{,GG},..." */

	if (gmt_M_is_verbose (API->GMT, GMT_MSG_DEBUG)) {
		char txt[4] = {""};
		for (k = 0; k < GMT_N_FAMILIES; k++) if (n_per_family[k] != GMT_NOTSET) {
			(n_per_family[k] == GMTAPI_UNLIMITED) ? sprintf (txt, ">1") : sprintf (txt, "%d", n_per_family[k]);
			GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: For %s we expect %s input objects\n", GMT_family[k], txt);
		}
	}

	/* 2b. Make some specific modifications to the keys given the options passed */
	if (deactivate_output && (k = api_get_key (API, GMT_OPT_OUTFILE, key, n_keys)) >= 0)
		key[k][K_DIR] = api_not_required_io (key[k][K_DIR]);	/* Since an explicit output file already specified or not required */

	/* 3. Count command line output files */
	for (opt = *head; opt; opt = opt->next)
		if (opt->option == GMT_OPT_OUTFILE) n_out++;
	if (n_out > 1) {
		GMT_Report (API, GMT_MSG_NORMAL, "GMT_Encode_Options: Can only specify one main output object via command line\n");
		return_null (NULL, GMT_ONLY_ONE_ALLOWED);	/* Too many output objects */
	}
	n_alloc = n_keys;	/* Initial number of allocations */
	info = calloc (n_alloc, sizeof (struct GMT_RESOURCE));

	/* 4. Determine position of file args given as $ or via missing arg (proxy for input matrix) */
	/* Note: All explicit objects must be given after all implicit matrices have been listed */
	for (opt = *head; opt; opt = opt->next) {	/* Process options */
		strip = (strip_colon_opt == opt->option) ? strip_colon : false;	/* Just turn strip possibly true for the relevant option */
		k = api_get_key (API, opt->option, key, n_keys);	/* If k >= 0 then this option is among those listed in the keys array */
		family = geometry = GMT_NOTSET;	/* Not set yet */
		if (k >= 0) {	/* Got a key, so split out family and geometry flags */
			sdir = api_key_to_family (API, key[k], &family, &geometry);	/* Get dir, datatype, and geometry */
			if (sdir < 0) {	/* Could not determine direction */
				GMT_Report (API, GMT_MSG_NORMAL, "GMT_Encode_Options: Key not understood so direction is undefined? Notify developers\n");
				sdir = GMT_IN;
			}
			direction = (unsigned int) sdir;
		}
		mod_pos = api_extract_argument (opt->arg, argument, key, k, strip, &n_pre_arg);	/* Pull out the option argument, possibly modified by the key */
		if (api_found_marker (argument)) {	/* Found an explicit questionmark within the option, e.g., -G?, -R? or -<? */
			if (opt->option == 'R' && !strcmp (opt->arg, "?")) {	/* -R? means append a grid so set those parameters here */
				GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Option -R? found: explicit grid will be substituted\n");
				family = GMT_IS_GRID;
				geometry = GMT_IS_SURFACE;
				direction = GMT_IN;
			}
			else if (k == GMT_NOTSET) {	/* Found questionmark but no corresponding key found? */
				GMT_Report (API, GMT_MSG_NORMAL, "GMT_Encode_Options: Error: Got a -<option>? argument but not listed in keys\n");
				direction = GMT_IN;	/* Have to assume it is an input file if not specified */
			}
			info[n_items].mode = (k >= 0 && api_is_required_IO (key[k][K_DIR])) ? K_PRIMARY : K_SECONDARY;
			if (k >= 0 && key[k][K_DIR] != '-')
				key[k][K_DIR] = api_not_required_io (key[k][K_DIR]);	/* Make sure required { becomes ( and } becomes ) so we don't add them later */
			/* Add this item to our list */
			info[n_items].option    = opt;
			info[n_items].family    = family;
			info[n_items].geometry  = geometry;
			info[n_items].direction = direction;
			info[n_items].pos = pos = (direction == GMT_IN) ? input_pos++ : output_pos++;	/* Explicitly given arguments are the first given on the r.h.s. */
			kind = GMT_FILE_EXPLICIT;
			n_items++;
			if (direction == GMT_IN) n_in_added++;
		}
		else if (k >= 0 && key[k][K_OPT] != GMT_OPT_INFILE && family != GMT_NOTSET && key[k][K_DIR] != '-') {	/* Got some option like -G or -Lu with further args */
			bool implicit = true;
			if ((len = strlen (argument)) == (size_t)n_pre_arg)	/* Got some option like -G or -Lu with no further args */
				GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Option -%c needs implicit arg [offset = %d]\n", opt->option, n_pre_arg);
			else if (mod_pos && (argument[mod_pos] == '\0' || argument[mod_pos] == '+'))	/* Found an embedded +q<noarg> */
				GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Option -%c needs implicit arg via argument-less +%c modifier\n", opt->option, key[k][K_MODIFIER]);
			else
				implicit = false;
			if (implicit) {
				/* This is an implicit reference and we must explicitly add the missing item by adding the questionmark */
				info[n_items].option    = opt;
				info[n_items].family    = family;
				info[n_items].geometry  = geometry;
				info[n_items].direction = direction;
				info[n_items].mode = (api_is_required_IO (key[k][K_DIR])) ? K_PRIMARY : K_SECONDARY;
				key[k][K_DIR] = api_not_required_io (key[k][K_DIR]);	/* Change to ( or ) since option was provided, albeit implicitly */
				info[n_items].pos = pos = (direction == GMT_IN) ? input_pos++ : output_pos++;
				/* Explicitly add the missing marker (e.g., ?) to the option argument */
				if (mod_pos) {	/* Must expand something like 300k+s+d+u into 300k+s$+d+u (assuming +s triggered this test) */
					strncpy (txt, opt->arg, mod_pos);
					strcat (txt, "?");
					if (opt->arg[mod_pos]) strcat (txt, &opt->arg[mod_pos]);
				}
				else if (strip)	/* Special case for quoted and decorated lines with colon separating label info */
					snprintf (txt, GMT_LEN256, "%s?%s", argument, &opt->arg[2]);
				else if (n_pre_arg)	/* Something like -Lu becomes -Lu$ */
					snprintf (txt, GMT_LEN256, "%s?", opt->arg);
				else	/* Something like -C or -C+d200k becomes -C$ or -C$+d200k */
					snprintf (txt, GMT_LEN256, "?%s", opt->arg);
				gmt_M_str_free (opt->arg);
				opt->arg = strdup (txt);
				kind = GMT_FILE_EXPLICIT;
				n_items++;
				if (direction == GMT_IN) n_in_added++;
			}
			else {	/* No implicit file argument involved, just check if this satisfies a required option */
				kind = GMT_FILE_NONE;
				if (k >= 0) {	/* If this was a required input|output it has now been satisfied */
					/* Add check to make sure argument for input is an existing file! */
					key[k][K_DIR] = api_not_required_io (key[k][K_DIR]);	/* Change to ( or ) since option was provided, albeit implicitly */
					satisfy = special_text[direction];
				}
				else	/* Nothing special about this option */
					satisfy = special_text[2];
			}
		}
		else {	/* No implicit file argument involved, just check if this satisfies a required option */
			kind = GMT_FILE_NONE;
			if (k >= 0) {	/* If this was a required input|output it has now been satisfied */
				/* Add check to make sure argument for input is an existing file! */
				key[k][K_DIR] = api_not_required_io (key[k][K_DIR]);	/* Change to ( or ) since option was provided, albeit implicitly */
				satisfy = special_text[direction];
			}
			else	/* Nothing special about this option */
				satisfy = special_text[2];
		}
		if (kind == GMT_FILE_EXPLICIT)
			GMT_Report (API, GMT_MSG_DEBUG, "%s: Option -%c%s includes a memory reference to %s argument # %d\n",
			            S[direction], opt->option, opt->arg, LR[direction], pos);
		else
			GMT_Report (API, GMT_MSG_DEBUG, "---: Option -%c%s includes no memory reference%s\n",
			            opt->option, opt->arg, satisfy);
		if (n_items == n_alloc) {
			n_alloc <<= 1;
			info = realloc (info, n_alloc * sizeof (struct GMT_RESOURCE));
		}
	}

	/* Done processing references that were explicitly given in the options.  Now determine if module
	 * has required input or output references that we must add (if not specified explicitly above) */

	for (ku = 0; ku < n_keys; ku++) {	/* Each set of keys specifies if the item is required via the 3rd key letter */
		if (api_is_required_IO (key[ku][K_DIR])) {	/* Required input|output that was not specified explicitly above */
			char str[2] = {'?',0};
			if ((sdir = api_key_to_family (API, key[ku], &family, &geometry)) == GMT_NOTSET) {	/* Extract family and geometry */
				GMT_Report (API, GMT_MSG_NORMAL, "Failure to extract family, geometry, and direction!!!!\n");
				continue;
			}
			direction = (unsigned int) sdir;
			/* We need to know how many implicit items for a given family we might have to add.  For instance,
			 * one can usually give any number of data or text tables but only one grid file.  However, this is
			 * not a fixed thing, hence we counted up n_per_family from the keys earlier so we have some limits */
			if (direction == GMT_OUT || n_in == GMT_NOTSET)	/* For output or lack of info we only add one item per key */
				n_to_add = 1;
			else	/* More information to act on for inputs */
				n_to_add = (n_per_family[family] == GMTAPI_UNLIMITED) ? n_in - n_in_added : n_per_family[family];
			for (e = 0; e < n_to_add; e++) {
				new_ptr = GMT_Make_Option (API, key[ku][K_OPT], str);	/* Create new option(s) with filename "$" */
				/* Append the new option to the list */
				*head = GMT_Append_Option (API, new_ptr, *head);
				info[n_items].option    = new_ptr;
				info[n_items].family    = family;
				info[n_items].geometry  = geometry;
				info[n_items].direction = direction;
				info[n_items].pos = (direction == GMT_IN) ? input_pos++ : output_pos++;
				info[n_items].mode = K_PRIMARY;
				GMT_Report (API, GMT_MSG_DEBUG, "%s: Must add -%c? as implicit memory reference to %s argument # %d\n",
					S[direction], key[ku][K_OPT], LR[direction], info[n_items].pos);
				n_items++;
				if (direction == GMT_IN) n_in_added++;
				if (n_items == n_alloc) {
					n_alloc <<= 1;
					info = realloc (info, n_alloc * sizeof (struct GMT_RESOURCE));
				}
			}
		}
		gmt_M_str_free (key[ku]);	/* Free up this key */
	}
	/* Free up the temporary key array */
	gmt_M_str_free (key);

	/* Reallocate the information structure array or remove entirely if nothing given. */
	if (n_items && n_items < n_alloc) info = realloc (info, n_items * sizeof (struct GMT_RESOURCE));
	else if (n_items == 0) gmt_M_str_free (info);	/* No containers used */

	gmt_M_memset (nn, 4, unsigned int);
	for (ku = 0; ku < n_items; ku++)	/* Count how many primary and secondary objects each for input and output */
		nn[info[ku].direction][info[ku].mode]++;

	for (ku = 0; ku < n_items; ku++) {	/* Reorder positions so that primary objects are listed before secondary objects */
		if (info[ku].mode == K_SECONDARY) info[ku].pos += nn[info[ku].direction][K_PRIMARY];	/* Move secondary objects after all primary objects for this direction */
		else info[ku].pos -= nn[info[ku].direction][K_SECONDARY];	/* Move any primary objects to start of list for this direction */
	}
	GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Found %d inputs and %d outputs that need memory hook-up\n", input_pos, output_pos);
	/* Just checking that the options were properly processed */
	if (gmt_M_is_verbose (API->GMT, GMT_MSG_DEBUG)) {
		static char *omode[2] = {"Primary", "Secondary"};
		text = GMT_Create_Cmd (API, *head);
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: Revised command before memory-substitution: %s\n", text);
		GMT_Destroy_Cmd (API, &text);
		GMT_Report (API, GMT_MSG_DEBUG, "GMT_Encode_Options: List of items returned:\n");
		for (ku = 0; ku < n_items; ku++) {
			GMT_Report (API, GMT_MSG_DEBUG, "External API item %2d: Family: %14s Direction: %6s Pos: %d Mode: %s\n",
				ku, GMT_family[info[ku].family], GMT_direction[info[ku].direction], info[ku].pos, omode[info[ku].mode]);
		}
	}

	/* Pass back the info array and the number of items */
	*n = (n_items == 0) ? UINT_MAX : n_items;	/* E.g., n_keys = 0 for gmtset, gmtdefaults, gmtlogo */
	return (info);
}

#ifdef FORTRAN_API
struct GMT_RESOURCE *GMT_Encode_Options_ (const char *module, int *n_in, struct GMT_OPTION **head, unsigned int *n, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Encode_Options (GMT_FORTRAN, module, *n_in, head, n));
}
#endif

/* Parsing API: to present, examine GMT Common Option current settings and GMT Default settings */

/*! . */
int GMT_Get_Common (void *V_API, unsigned int option, double par[]) {
	/* Inquires if specified GMT option has been set and obtains current values for some of them, if par is not NULL.
	 * Returns GMT_NOTSET if the option has not been specified.  Otherwise, returns the number of parameters
	 * it passed back via the par[] array.  Only some options passes back parameters; these are
	 * -R, -I, -X, -Y, -b, -f, -i, -o, -r, -t, -:, while the others return 0.
	 */
	int ret = GMT_NOTSET;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	GMT = API->GMT;

	switch (option) {
		case 'B':	if (GMT->common.B.active[0] || GMT->common.B.active[1]) ret = 0; break;
		case 'I':
			if (GMT->common.R.active[ISET]) {
				ret = 2;
				if (par) gmt_M_memcpy (par, GMT->common.R.inc, 2, double);
			}
			break;
		case 'J':	if (GMT->common.J.active) ret = 0; break;
		case 'K':	if (GMT->common.K.active) ret = 0; break;
		case 'O':	if (GMT->common.O.active) ret = 0; break;
		case 'P':	if (GMT->common.P.active) ret = 0; break;
		case 'R':
			if (GMT->common.R.active[RSET]) {
				ret = 4;
				if (par) gmt_M_memcpy (par, GMT->common.R.wesn, 4, double);
			}
			break;
		case 'U':	if (GMT->common.U.active) ret = 0; break;
		case 'V':	if (GMT->common.V.active) ret = GMT->current.setting.verbose; break;
		case 'X':
			if (GMT->common.X.active) {
				ret = 1;
				if (par) par[0] = GMT->common.X.off;
			}
			break;
		case 'Y':
			if (GMT->common.Y.active) {
				ret = 1;
				if (par) par[0] = GMT->common.Y.off;
			}
			break;
		case 'a':	if (GMT->common.a.active) ret = GMT->common.a.geometry; break;
		case 'b':	if (GMT->common.b.active[GMT_IN]) ret = GMT_IN; else if (GMT->common.b.active[GMT_OUT]) ret = GMT_OUT; break;
		case 'c':	if (GMT->common.c.active) ret = GMT->common.c.copies; break;
		case 'f':	if (GMT->common.f.active[GMT_IN]) ret = GMT_IN; else if (GMT->common.f.active[GMT_OUT]) ret = GMT_OUT; break;
		case 'g':	if (GMT->common.g.active) ret = 0; break;
		case 'h':	if (GMT->common.h.active) ret = GMT->common.h.mode; break;
		case 'i':	if (GMT->common.i.active) ret = (int)GMT->common.i.n_cols; break;
		case 'n':	if (GMT->common.n.active) ret = 0; break;
		case 'o':	if (GMT->common.o.active) ret = (int)GMT->common.o.n_cols; break;
		case 'p':	if (GMT->common.p.active) ret = 0; break;
		case 'r':	if (GMT->common.R.active[GSET]) ret = GMT->common.R.registration; break;
		case 's':	if (GMT->common.s.active) ret = 0; break;
		case 't':
			if (GMT->common.t.active) {
				ret = 1;
				if (par) par[0] = GMT->common.t.value;
			}
			break;
		case ':':	if (GMT->common.colon.toggle[GMT_IN]) ret = GMT_IN; else if (GMT->common.colon.toggle[GMT_OUT]) ret = GMT_OUT; break;
		default:
			gmtapi_report_error (API, GMT_OPTION_NOT_FOUND);
			break;
	}

	return (ret);
}

#ifdef FORTRAN_API
int GMT_Get_Common_ (unsigned int *option, double par[]) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Common (GMT_FORTRAN, *option, par));
}
#endif

/*! . */
int GMT_Get_Default (void *V_API, const char *keyword, char *value) {
	/* Given the text representation of a GMT parameter keyword, return its setting as text.
	 * value must have enough space for the return information.
	 */
	int error = GMT_NOERROR;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (keyword == NULL) return_error (V_API, GMT_NO_PARAMETERS);
	if (value == NULL) return_error (V_API, GMT_NO_PARAMETERS);
	API = api_get_api_ptr (V_API);
	/* First intercept any API Keywords */
	if (!strncmp (keyword, "API_VERSION", 11U))	/* The API version */
		sprintf (value, "%s", GMT_PACKAGE_VERSION);
	else if (!strncmp (keyword, "API_PAD", 7U))	/* Change the grid padding setting */
		sprintf (value, "%d", API->pad);
	else if (!strncmp (keyword, "API_BINDIR", 10U))	/* Report binary directory */
		sprintf (value, "%s", API->GMT->init.runtime_bindir);
	else if (!strncmp (keyword, "API_SHAREDIR", 12U))	/* Report share directory */
		sprintf (value, "%s", API->GMT->session.SHAREDIR);
	else if (!strncmp (keyword, "API_DATADIR", 12U))	/* Report data directory */
		sprintf (value, "%s", API->GMT->session.DATADIR);
	else if (!strncmp (keyword, "API_PLUGINDIR", 14U))	/* Report plugin directory */
		sprintf (value, "%s", API->GMT->init.runtime_plugindir);
	else if (!strncmp (keyword, "API_LIBRARY", 11U))	/* Report core library */
		sprintf (value, "%s", API->GMT->init.runtime_library);
	else if (!strncmp (keyword, "API_CORES", 9U))	/* Report number of cores */
		sprintf (value, "%d", API->n_cores);
#ifdef HAVE_GDAL
	else if (!strncmp (keyword, "API_IMAGE_LAYOUT", 16U))	/* Report image/band layout */
		gmt_M_memcpy (value, API->GMT->current.gdal_read_in.O.mem_layout, 4, char);
#endif
	else if (!strncmp (keyword, "API_GRID_LAYOUT", 15U)) {	/* Report grid layout */
		if (API->shape == GMT_IS_COL_FORMAT)
			strcpy (value, "columns");
		else if (API->shape == GMT_IS_ROW_FORMAT)
			strcpy (value, "rows");
	}
	else {	/* Must process as a GMT setting */
		strcpy (value, gmtlib_putparameter (API->GMT, keyword));
		error = (value[0] == '\0') ? GMT_OPTION_NOT_FOUND : GMT_NOERROR;
	}
	return_error (V_API, error);
}

#ifdef FORTRAN_API
int GMT_Get_Default_ (char *keyword, char *value, int len1, int len2) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Default (GMT_FORTRAN, keyword, value));
}
#endif

/*! . */
int GMT_Set_Default (void *V_API, const char *keyword, const char *txt_val) {
	/* Given the text representation of a GMT or API parameter keyword, assign its value.
	 */
	unsigned int error = GMT_NOERROR;
	struct GMTAPI_CTRL *API = NULL;
	char *value = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (keyword == NULL) return_error (V_API, GMT_NOT_A_VALID_PARAMETER);
	if (txt_val == NULL) return_error (V_API, GMT_NO_PARAMETERS);
	API = api_get_api_ptr (V_API);
	value = strdup (txt_val);	/* Make local copy to be safe */
	/* First intercept any API Keywords */
	if (!strncmp (keyword, "API_PAD", 7U)) {	/* Change the grid padding setting */
		int pad = atoi (value);
		if (pad >= 0) {
			gmt_set_pad (API->GMT, pad);	/* Change the default pad; give GMT_NOTSET to leave as is */
			API->pad = pad;
		}
	}
#ifdef HAVE_GDAL
	else if (!strncmp (keyword, "API_IMAGE_LAYOUT", 16U)) {	/* Change image/band layout */
		if (strlen (value) != 4U) {
			error = 1;
			GMT_Report (API, GMT_MSG_NORMAL, "API_IMAGE_LAYOUT requires a 4-character specification. %s is ignored",  value);
		}
		else
			gmt_M_memcpy (API->GMT->current.gdal_read_in.O.mem_layout, value, 4, char);
	}
#endif
	else if (!strncmp (keyword, "API_GRID_LAYOUT", 15U)) {	/* Change grid layout */
		if (!strncmp (keyword, "columns", 7U))
			API->shape = GMT_IS_COL_FORMAT;	/* Switch to column-major format */
		else if (!strncmp (keyword, "rows", 4U))
			API->shape = GMT_IS_ROW_FORMAT;	/* Switch to row-major format */
		else
			GMT_Report (API, GMT_MSG_NORMAL, "API_GRID_LAYOUT must be either \"columns\" or \"rows\"",  value);
		error = 1;
	}
	else	/* Must process as a GMT setting */
		error = gmtlib_setparameter (API->GMT, keyword, value, false);
	gmt_M_str_free (value);
	return_error (V_API, (error) ? GMT_NOT_A_VALID_PARAMETER : GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Set_Default_ (char *keyword, char *value, int len1, int len2) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Set_Default (GMT_FORTRAN, keyword, value));
}
#endif

/*! . */
int GMT_Option (void *V_API, const char *options) {
	/* Take comma-separated GMT options and print the corresponding usage message(s). */
	unsigned int pos = 0, k = 0, n = 0;
	char p[GMT_LEN64] = {""}, arg[GMT_LEN64] = {""};
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (options == NULL) return_error (V_API, GMT_NO_PARAMETERS);
	API = api_get_api_ptr (V_API);

	/* The following does the translation between the rules for the option string and the convoluted items gmtlib_explain_options expects. */
	while (gmt_strtok (options, ",", &pos, p) && k < (GMT_LEN64-1)) {
		switch (p[0]) {
			case 'B':	/* Let B be B and B- be b */
				arg[k++] = (p[1] == '-') ? 'b' : 'B';
				break;
			case 'J':	/* Let J be -J and J- be j, JX is -Jx|X only, and -J[-]3 be adding -Z for 3-D scaling */
				n = 1;
				if (p[1] == '-') { arg[k++] = 'j'; n++; }
				else if (p[1] == 'X') { arg[k++] = 'x'; n++; }
				else arg[k++] = 'J';
				if (p[n] == 'Z' || p[n] == 'z') arg[k++] = 'Z';
				break;
			case 'R':	/* Want -R region usage */
				if (p[1]) {	/* Gave modifiers */
					if (p[1] == 'x') arg[k++] = 'S';	/* CarteSian region */
					else if (p[1] == 'g') arg[k++] = 'G';	/* Geographic region */
					else arg[k++] = 'R';			/* Generic region [Default] */
					if (p[1] == '3' || p[2] == '3') arg[k++] = 'z';	/* 3-D region */
				}
				else arg[k++] = 'R';			/* Generic region [Default] */
				break;
			case 'b':	/* Binary i/o -bi -bo */
				arg[k++] = (p[1] == 'i') ? 'C' : 'D';
				arg[k++] = (p[2]) ? p[2] : '0';
				break;
			case 'd':	/* Nodata flag -d, -di, -do */
				if (p[1] == 'i') arg[k++] = 'k';
				else if (p[1] == 'o') arg[k++] = 'l';
				else arg[k++] = 'd';
				break;
			case 'r':	/* Pixel registration */
				arg[k++] = 'F';
				break;
			case 'x':	/* Number of threads (for multi-threaded progs) */
				arg[k++] = 'y';
				break;
			default:	/* All others are pass-through */
				arg[k++] = p[0];
				break;
		}
	}
	gmtlib_explain_options (API->GMT, arg);	/* Call the underlying explain_options machinery */
	return_error (V_API, GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Option_ (void *V_API, char *options, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Option (GMT_FORTRAN, options));
}
#endif

/*! . */
int GMT_Message (void *V_API, unsigned int mode, const char *format, ...) {
	/* Message independent of verbosity, optionally with timestamp.
	 * mode = 0:	No time stamp
	 * mode = 1:	Abs time stamp formatted via GMT_TIME_STAMP
	 * mode = 2:	Report elapsed time since last reset.
	 * mode = 4:	Reset elapsed time to 0, no time stamp.
	 * mode = 6:	Reset elapsed time and report it as well.
	 */
	size_t source_info_len;
	char message[4*GMT_BUFSIZ] = {""}, *stamp = NULL;
	struct GMTAPI_CTRL *API = NULL;
	va_list args;

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (format == NULL) return GMT_PTR_IS_NULL;	/* Format cannot be NULL */
	API = api_get_api_ptr (V_API);	/* Get the typecast structure pointer to API */
	if (mode) stamp = api_tictoc_string (API, mode);	/* Pointer to a timestamp string */
	if (mode % 4) sprintf (message, "%s | ", stamp);	/* Lead with the time stamp */
	source_info_len = strlen (message);

	va_start (args, format);
	vsnprintf (message + source_info_len, 4*GMT_BUFSIZ - source_info_len, format, args);
	va_end (args);
	assert (strlen (message) < 4*GMT_BUFSIZ);
	API->print_func (API->GMT->session.std[GMT_ERR], message);	/* Do the printing */
	return_error (V_API, GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Message_ (void *V_API, unsigned int *mode, const char *message, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Message (GMT_FORTRAN, *mode, message));
}
#endif

/*! . */
int GMT_Report (void *V_API, unsigned int level, const char *format, ...) {
	/* Message whose output depends on verbosity setting */
	size_t source_info_len = 0;
	unsigned int g_level;
	char message[GMT_BUFSIZ] = {""};
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;
	va_list args;
	/* GMT_Report may be called before GMT is set so take precautions */
	if (V_API == NULL) return GMT_NOERROR;		/* Not a fatal issue here but we cannot report anything */
	API = api_get_api_ptr (V_API);	/* Get the typecast structure pointer to API */
	GMT = API->GMT;	/* Short-hand for the GMT sub-structure */
	g_level = (GMT) ? GMT->current.setting.verbose : 0;
	if (level > MAX(API->verbose, g_level))
		return 0;
	if (format == NULL) return GMT_PTR_IS_NULL;	/* Format cannot be NULL */
	if (GMT && GMT->current.setting.timer_mode > GMT_NO_TIMER) {
		char *stamp = api_tictoc_string (API, GMT->current.setting.timer_mode);	/* NULL or pointer to a timestamp string */
		if (stamp) {
			sprintf (message, "%s | ", stamp);	/* Lead with the time stamp */
			source_info_len = strlen (message);	/* Update length of message from 0 */
		}
	}
	snprintf (message + source_info_len, GMT_BUFSIZ-source_info_len, "%s: ", (GMT && GMT->init.module_name) ? GMT->init.module_name : API->session_tag);
	source_info_len = strlen (message);
	va_start (args, format);
	/* append format to the message: */
	vsnprintf (message + source_info_len, GMT_BUFSIZ - source_info_len, format, args);
	va_end (args);
	assert (strlen (message) < GMT_BUFSIZ);
	API->print_func (GMT ? GMT->session.std[GMT_ERR] : stderr, message);
	return_error (V_API, GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Report_ (void *V_API, unsigned int *level, const char *format, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Report (GMT_FORTRAN, *level, format));
}
#endif

/*! . */
int GMT_Get_Values (void *V_API, const char *arg, double par[], int maxpar) {
	/* Parse any number of comma, space, tab, semi-colon or slash-separated values.
	 * The array par must have enough space to hold a maximum of maxpar items.
	 * Function returns the number of items, or GMT_NOTSET if there was an error.
	 * When there are more than maxpar items, only the first maxpar are stored, and
	 * the value of maxpar is returned.
	 * We can handle dimension units (c|i|p), distance units (d|m|s|e|f|k|M|n|u),
	 * geographic coordinates, absolute dateTtime strings, and regular floats.
	 *
	 * Dimensions are returned in the current length unit [inch or cm].
	 * Distances are returned in meters.
	 * Arc distances are returned in degrees.
	 * Geographic dd:mm:ss[W|E|S|N] coordinates are returned in decimal degrees.
	 * DateTtime moments are returned in time in chosen units [sec] since chosen epoch [1970] */

	unsigned int pos = 0, mode, col_type_save[2][2];
	int npar = 0;
	size_t len;
	char p[GMT_BUFSIZ] = {""}, unit;
	double value;
	struct GMTAPI_CTRL *API = NULL;
	struct GMT_CTRL *GMT = NULL;
	static const char separators[] = " \t,;/";

	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	if (arg == NULL || arg[0] == '\0') return_value (V_API, GMT_NO_PARAMETERS, GMT_NOTSET);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	GMT = API->GMT;

	/* Because gmt_init_distaz and possibly gmt_scanf_arg may decide to change the GMT col_type
	 * we make a copy here and reset when done */
	gmt_M_memcpy (col_type_save[GMT_IN], GMT->current.io.col_type[GMT_IN],   2, unsigned int);
	gmt_M_memcpy (col_type_save[GMT_OUT], GMT->current.io.col_type[GMT_OUT], 2, unsigned int);

	while (gmt_strtok (arg, separators, &pos, p)) {	/* Loop over input aruments */
		if ((len = strlen (p)) == 0) continue;
		if (npar >= maxpar) {	/* Bail out when already maxpar values are stored */
			gmtapi_report_error (API, GMT_DIM_TOO_LARGE);
			break;
		}
		len--;	/* Position of last char, possibly a unit */
		if (strchr (GMT_DIM_UNITS, p[len]))	/* Dimension unit (c|i|p), return distance in GMT default length unit [inch or cm] */
			value = gmt_convert_units (GMT, p, GMT->current.setting.proj_length_unit, GMT->current.setting.proj_length_unit);
		else if (strchr (GMT_LEN_UNITS, p[len])) {	/* Distance units, return as meters [or degrees if arc] */
			mode = gmt_get_distance (GMT, p, &value, &unit);
			gmt_init_distaz (GMT, unit, mode, GMT_MAP_DIST);
			value /= GMT->current.map.dist[GMT_MAP_DIST].scale;	/* Convert to default unit */
		}
		else	/* Perhaps coordinates or floats */
			(void) gmt_scanf_arg (GMT, p, GMT_IS_UNKNOWN, &value);
		par[npar++] = value;
	}
	/* Reset col_types to what they were before the parsing */
	gmt_M_memcpy (GMT->current.io.col_type[GMT_IN],  col_type_save[GMT_IN],  2, unsigned int);
	gmt_M_memcpy (GMT->current.io.col_type[GMT_OUT], col_type_save[GMT_OUT], 2, unsigned int);

	return (npar);
}

#ifdef FORTRAN_API
int GMT_Get_Values_ (char *arg, double par[], int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Values (GMT_FORTRAN, arg, par, len));
}
#endif

/* Here lies the very basic F77 support for grid read and write only. It is assumed that no grid padding is required */

#define F_STRNCPY(dst,src,ldst,lsrc) { int l = MIN(ldst-1, lsrc); strncpy (dst, src, l); dst[l] = '\0'; }

int gmt_f77_readgrdinfo_ (unsigned int dim[], double limit[], double inc[], char *title, char *remark, const char *name, int ltitle, int lremark, int lname) {
	/* Note: When returning, dim[2] holds the registration (0 = gridline, 1 = pixel).
	 * limit[4-5] holds zmin/zmax. limit must thus at least have a length of 6.
	 */
	const char *argv = "GMT_F77_readgrdinfo";
	char *file = NULL;
	struct GMT_GRID_HEADER header;
	struct GMTAPI_CTRL *API = NULL;	/* The API pointer assigned below */

	if (name == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "No filename given to GMT_F77_readgrdinfo\n");
		return GMT_ARG_IS_NULL;
	}
	if ((API = GMT_Create_Session (argv, 0U, 0U, NULL)) == NULL) return GMT_MEMORY_ERROR;
	file = strndup (name, lname);

	/* Read the grid header */

	gmt_M_memset (&header, 1, struct GMT_GRID_HEADER);	/* To convince Coverity that header->index_function has been initialized */
	if (gmtlib_read_grd_info (API->GMT, file, &header)) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error opening file %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_GRID_READ_ERROR;
	}
	gmt_M_str_free (file);

	/* Assign variables from header structure items */
	dim[GMT_X] = header.n_columns;	dim[GMT_Y] = header.n_rows;
	gmt_M_memcpy (limit, header.wesn, 4U, double);
	gmt_M_memcpy (inc, header.inc, 2U, double);
	limit[ZLO] = header.z_min;
	limit[ZHI] = header.z_max;
	dim[GMT_Z] = header.registration;
	if (title) F_STRNCPY (title, header.title, ltitle, GMT_GRID_TITLE_LEN80);
	if (remark) F_STRNCPY (remark, header.remark, lremark, GMT_GRID_REMARK_LEN160);

	if (GMT_Destroy_Session (API) != GMT_NOERROR) return GMT_RUNTIME_ERROR;
	return GMT_NOERROR;
}

int gmt_f77_readgrd_ (float *array, unsigned int dim[], double limit[], double inc[], char *title, char *remark, const char *name, int ltitle, int lremark, int lname) {
	/* Note: When called, dim[2] is 1 we allocate the array, otherwise we assume it has enough space
	 * Also, if dim[3] == 1 then we transpose the array before writing.
	 * When returning, dim[2] holds the registration (0 = gridline, 1 = pixel).
	 * limit[4-5] holds zmin/zmax. limit must thus at least have a length of 6.
	 */
 	unsigned int no_pad[4] = {0, 0, 0, 0};
	double no_wesn[4] = {0.0, 0.0, 0.0, 0.0};
	const char *argv = "GMT_F77_readgrd";
	char *file = NULL;
	struct GMT_GRID_HEADER header;
	struct GMTAPI_CTRL *API = NULL;	/* The API pointer assigned below */

	if (name == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "No filename given to GMT_F77_readgrd\n");
		return GMT_ARG_IS_NULL;
	}
	if ((API = GMT_Create_Session (argv, 0U, 0U, NULL)) == NULL) return GMT_MEMORY_ERROR;
	file = strndup (name, lname);

	/* Read the grid header */
	gmt_M_memset (&header, 1, struct GMT_GRID_HEADER);	/* To convince Coverity that header->index_function has been initialized */
	gmt_grd_init (API->GMT, &header, NULL, false);
	if (gmtlib_read_grd_info (API->GMT, file, &header)) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error opening file %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_GRID_READ_ERROR;
	}

	/* Read the grid, possibly after first allocating array space */
	if (dim[GMT_Z] == 1) array = gmt_M_memory (API->GMT, NULL, header.size, float);
	if (gmtlib_read_grd (API->GMT, file, &header, array, no_wesn, no_pad, 0)) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error reading file %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_GRID_READ_ERROR;
	}
	gmt_M_str_free (file);

	if (dim[3] == 1) gmtlib_inplace_transpose (array, header.n_rows, header.n_columns);

	/* Assign variables from header structure items */
	dim[GMT_X] = header.n_columns;	dim[GMT_Y] = header.n_rows;
	gmt_M_memcpy (limit, header.wesn, 4U, double);
	gmt_M_memcpy (inc, header.inc, 2U, double);
	limit[ZLO] = header.z_min;
	limit[ZHI] = header.z_max;
	dim[GMT_Z] = header.registration;
	if (title) F_STRNCPY (title, header.title, ltitle, GMT_GRID_TITLE_LEN80);
	if (remark) F_STRNCPY (remark, header.remark, lremark, GMT_GRID_REMARK_LEN160);

	if (GMT_Destroy_Session (API) != GMT_NOERROR) return GMT_RUNTIME_ERROR;
	return GMT_NOERROR;
}

int gmt_f77_writegrd_ (float *array, unsigned int dim[], double limit[], double inc[], const char *title, const char *remark, const char *name, int ltitle, int lremark, int lname) {
	/* Note: When called, dim[2] holds the registration (0 = gridline, 1 = pixel).
	 * Also, if dim[3] == 1 then we transpose the array before writing.  */
 	unsigned int no_pad[4] = {0, 0, 0, 0};
	const char *argv = "GMT_F77_writegrd";
	char *file = NULL;
	double no_wesn[4] = {0.0, 0.0, 0.0, 0.0};
	struct GMT_GRID_HEADER header;
	struct GMTAPI_CTRL *API = NULL;	/* The API pointer assigned below */

	/* Initialize with default values */

	if (name == NULL) {
		GMT_Report (API, GMT_MSG_NORMAL, "No filename given to GMT_F77_writegrd\n");
		return GMT_ARG_IS_NULL;
	}
	if ((API = GMT_Create_Session (argv, 0U, 0U, NULL)) == NULL) return GMT_MEMORY_ERROR;
	file = strndup (name, lname);

	gmt_M_memset (&header, 1, struct GMT_GRID_HEADER);	/* To convince Coverity that header->index_function has been initialized */
	gmt_grd_init (API->GMT, &header, NULL, false);
	if (full_region (limit)) {	/* Here that means limit was not properly given */
		GMT_Report (API, GMT_MSG_NORMAL, "Grid domain not specified for %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_ARG_IS_NULL;
	}
	if (inc[GMT_X] == 0.0 || inc[GMT_Y] == 0.0) {	/* Here that means grid spacing was not properly given */
		GMT_Report (API, GMT_MSG_NORMAL, "Grid spacing not specified for %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_ARG_IS_NULL;
	}

	/* Set header parameters */

	gmt_M_memcpy (header.wesn, limit, 4U, double);
	gmt_M_memcpy (header.inc, inc, 2U, double);
	header.n_columns = dim[GMT_X];	header.n_rows = dim[GMT_Y];
	header.registration = dim[GMT_Z];
	gmt_set_grddim (API->GMT, &header);
	if (title) F_STRNCPY (header.title, title, GMT_GRID_TITLE_LEN80, ltitle);
	if (remark) F_STRNCPY (header.remark, remark, GMT_GRID_REMARK_LEN160, lremark);

	if (dim[3] == 1) gmtlib_inplace_transpose (array, header.n_rows, header.n_columns);

	/* Write the file */

	if (gmtlib_write_grd (API->GMT, file, &header, array, no_wesn, no_pad, 0)) {
		GMT_Report (API, GMT_MSG_NORMAL, "Error writing file %s\n", file);
		gmt_M_str_free (file);
		GMT_Destroy_Session (API);
		return GMT_GRID_WRITE_ERROR;
	}
	gmt_M_str_free (file);

	if (GMT_Destroy_Session (API) != GMT_NOERROR) return GMT_MEMORY_ERROR;
	return GMT_NOERROR;
}

/* wrappers for several Fortran compilers */
#define F77_ARG1 unsigned int dim[], double limit[], double inc[], char *title, char *remark, const char *name, int ltitle, int lremark, int lname
#define F77_ARG2 dim, limit, inc, title, remark, name, ltitle, lremark, lname
int gmt_f77_readgrdinfo__(F77_ARG1) { return gmt_f77_readgrdinfo_ (F77_ARG2); }
int gmt_f77_readgrdinfo  (F77_ARG1) { return gmt_f77_readgrdinfo_ (F77_ARG2); }
int GMT_F77_READGRDINFO_ (F77_ARG1) { return gmt_f77_readgrdinfo_ (F77_ARG2); }
int GMT_F77_READGRDINFO  (F77_ARG1) { return gmt_f77_readgrdinfo_ (F77_ARG2); }
#undef  F77_ARG1
#undef  F77_ARG2

#define F77_ARG1 float *array, unsigned int dim[], double limit[], double inc[], char *title, char *remark, const char *name, int ltitle, int lremark, int lname
#define F77_ARG2 array, dim, limit, inc, title, remark, name, ltitle, lremark, lname
int gmt_f77_readgrd__ (F77_ARG1) { return gmt_f77_readgrd_ (F77_ARG2); }
int gmt_f77_readgrd   (F77_ARG1) { return gmt_f77_readgrd_ (F77_ARG2); }
int GMT_F77_READGRD_  (F77_ARG1) { return gmt_f77_readgrd_ (F77_ARG2); }
int GMT_F77_READGRD   (F77_ARG1) { return gmt_f77_readgrd_ (F77_ARG2); }
#undef  F77_ARG1

#define F77_ARG1 float *array, unsigned int dim[], double limit[], double inc[], const char *title, const char *remark, const char *name, int ltitle, int lremark, int lname
int gmt_f77_writegrd__ (F77_ARG1) { return gmt_f77_writegrd_ (F77_ARG2); }
int gmt_f77_writegrd   (F77_ARG1) { return gmt_f77_writegrd_ (F77_ARG2); }
int GMT_F77_WRITEGRD_  (F77_ARG1) { return gmt_f77_writegrd_ (F77_ARG2); }
int GMT_F77_WRITEGRD   (F77_ARG1) { return gmt_f77_writegrd_ (F77_ARG2); }
#undef  F77_ARG1
#undef  F77_ARG2

char *GMT_Duplicate_String (void *API, const char* string) {
	/* Duplicate a string. The interest of this function is to make the memory allocation
	   inside the GMT lib so that GMT_Destroy_Data we can free it without any concerns of
	   Windows DLL hell */
	gmt_M_unused(API);
	return strdup (string);
}

/* Help functions specific to the Julia/GMT API */

EXTERN_MSC int GMT_blind_change_struct(void *V_API, void *ptr, void *what, char *type, size_t off);
int GMT_blind_change_struct(void *V_API, void *ptr, void *what, char *type, size_t off) {
	/* This is a magic backdoor to change static members of API structures that had to be declared as
	   immutables types in Julia and therefore impossible to change from within Julia.
	   *ptr  -> structure pointer whose member identified by the offset 'off' is to be changed.
	   *what -> pointer to the new value of the struct member that will be changed.
	   *type -> string with the type description, using the Julia types names. e.g. 'UInt32' or 'Float64'
	   The offset value 'off' is that obtained with the Julia's fieldoffsets() function, which is
	   equivalent to the 'offsetof()' C macro.
	*/
	if (!strcmp(type, "Int32"))
		*(int *)((char *)ptr + off) = *(int *)what;
	else if (!strcmp(type, "UInt32"))
		*(unsigned int *)((char *)ptr + off) = *(unsigned int *)what;
	else if (!strcmp(type, "Int64"))
		*(int64_t *)((char *)ptr + off) = *(int64_t *)what;
	else if (!strcmp(type, "UInt64"))
		*(uint64_t *)((char *)ptr + off) = *(uint64_t *)what;
	else if (!strcmp(type, "Float32"))
		*(float *)((char *)ptr + off) = *(float *)what;
	else if (!strcmp(type, "Float64"))
		*(double *)((char *)ptr + off) = *(double *)what;
	else if (!strcmp(type, "Int16"))
		*(signed short *)((char *)ptr + off) = *(signed short *)what;
	else if (!strcmp(type, "UInt16"))
		*(unsigned short *)((char *)ptr + off) = *(unsigned short *)what;
	else if (!strcmp(type, "UInt8"))
		*(unsigned char *)((char *)ptr + off) = *(unsigned char *)what;
	else if (!strcmp(type, "Int8"))
		*(char *)((char *)ptr + off) = *(char *)what;
	else {
		GMT_Report(V_API, GMT_MSG_NORMAL, "GMT/Julia Backdoor: Type (%s) not accepted. Possibly a pointer to something.\n", type);
		return_error (V_API, GMT_NOT_A_VALID_PARAMETER);
	}
	return GMT_NOERROR;
}

/* Sub-functions to perform specific conversions */

#define do_tbl_header(flag) (flag == 0 || flag == 2)
#define do_seg_header(flag) (flag == 0 || flag == 1)

/* GMT_DATASET to GMT_* : */

GMT_LOCAL void *api_dataset2dataset (struct GMTAPI_CTRL *API, struct GMT_DATASET *In, struct GMT_DATASET *Out, unsigned int header, unsigned int mode) {
	/* Convert a dataset to another dataset using current formatting and column type information.
	 * If Out is not NULL then we assume it has exact same dimension as the dataset, but no headers/records allocated.
	 * header controls what we do with headers.
	 * If mode == GMT_WRITE_TABLE_SEGMENT then we combine all segments into a SINGLE segment in ONE table
	 * If mode == GMT_WRITE_TABLE then we collect all segments into ONE table.
	 * If mode == GMT_WRITE_SEGMENT then we combine segments into ONE segment per table.
	 */
	unsigned int hdr;
	uint64_t tbl, seg, row, col, n_rows, tbl_out = 0, row_out = 0, seg_out = 0;
	bool s_alloc, t_alloc, alloc, was;
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATATABLE *Din = NULL;
	struct GMT_DATATABLE *Dout = NULL;
	struct GMT_DATASEGMENT *Sin = NULL;
	struct GMT_DATASEGMENT *Sout = NULL;
	s_alloc = t_alloc = alloc = (Out == NULL);
	if (alloc) {	/* Must allocate output textset */
		Out = gmt_M_memory (GMT, NULL, 1, struct GMT_DATASET);
		Out->n_tables = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : In->n_tables;
		Out->table = gmt_M_memory (GMT, NULL, Out->n_tables, struct GMT_DATATABLE *);
	}
	was = GMT->current.setting.io_header[GMT_OUT];
	GMT->current.setting.io_header[GMT_OUT] = do_tbl_header (header);
	Out->n_segments = (mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : ((mode == GMT_WRITE_SEGMENT) ? In->n_tables : In->n_segments);
	Out->n_records  = In->n_records;
	Out->n_columns  = In->n_columns;
	for (tbl = 0; tbl < In->n_tables; tbl++) {
		if (mode == GMT_WRITE_SEGMENT) row_out = 0;	/* Reset row output counter on a per table basis */
		else if (mode == 0) seg_out = 0;	/* Reset segment output counter on a per table basis */
		if (alloc && (mode == 0 || mode == GMT_WRITE_SEGMENT)) s_alloc = true;	/* Need to allocate at least one segment per table */
		Din = In->table[tbl];	/* Shorthand to current input data table */
		if (t_alloc) {
			Out->table[tbl_out] = Dout = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
			Dout->n_segments = Dout->n_alloc = (mode == GMT_WRITE_TABLE_SEGMENT || mode == GMT_WRITE_SEGMENT) ? 1 : ((mode == GMT_WRITE_TABLE) ? In->n_segments : Din->n_segments);	/* Number of segments in this table */
			Dout->n_records  = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : Din->n_records;	/* Number of data records int this table */
			Dout->n_columns = In->n_columns;
		}
		else
			Dout = Out->table[tbl_out];
		if (t_alloc) {
			if (Din->n_headers && do_tbl_header(header)) {	/* Allocate and duplicate headers */
				Dout->n_headers = Din->n_headers;	/* Same number of header records as input table */
				if (alloc) Dout->header = gmt_M_memory (GMT, NULL, Dout->n_headers, char *);
				for (hdr = 0; hdr < Dout->n_headers; hdr++) Dout->header[hdr] = strdup (Din->header[hdr]);
			}
			Dout->segment = gmt_M_memory (GMT, NULL, Dout->n_segments, struct GMT_DATASEGMENT *);
		}
		for (seg = 0; seg < Din->n_segments; seg++) {	/* For each input table segment */
			if (mode == 0 || mode == GMT_WRITE_TABLE) row_out = 0;	/* Reset row output counter on a per segment basis */
			Sin = Din->segment[seg];	/* Shorthand to current data segment */
			if (s_alloc) {	/* Allocate another segment */
				n_rows = (mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : ((mode == GMT_WRITE_SEGMENT) ? Din->n_records : Sin->n_rows);
				Dout->segment[seg_out] = GMT_Alloc_Segment (API, GMT_IS_DATASET, n_rows, In->n_columns, NULL, NULL);
				Sout = Dout->segment[seg_out];	/* Shorthand to current text segment */
				if (Sin->header && do_seg_header(header)) Sout->header = strdup (Sin->header);
			}
			else
				Sout = Dout->segment[seg_out];
			for (row = 0; row < Sin->n_rows; row++, row_out++) {	/* Copy each row to (new) segment */
				for (col = 0; col < Sin->n_columns; col++)
					Sout->data[col][row_out] = Sin->data[col][row];
			}
			if (mode == GMT_WRITE_SEGMENT || mode == GMT_WRITE_TABLE_SEGMENT) s_alloc = false;	/* Only allocate this single segment, at least for this table */
			if (mode == 0 || mode == GMT_WRITE_TABLE) seg_out++;	/* More than one segment on output */
		}
		if (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) t_alloc = false;	/* Only allocate this single table */
		if (mode == 0 || mode == GMT_WRITE_SEGMENT) tbl_out++;	/* More than one segment on output */
	}
	GMT->current.setting.io_header[GMT_OUT] = was;
	return Out;
}

GMT_LOCAL void *api_dataset2textset (struct GMTAPI_CTRL *API, struct GMT_DATASET *In, struct GMT_TEXTSET *Out, unsigned int header, unsigned int mode) {
	/* Convert a dataset to a textset using current formatting and column type information.
	 * If Out is not NULL then we assume it has exact same dimension as the dataset, but no headers/records allocated.
	 * header controls what we do with headers.
	 * If mode == GMT_WRITE_TABLE_SEGMENT then we combine all segments into a SINGLE segment in ONE table
	 * If mode == GMT_WRITE_TABLE then we collect all segments into ONE table.
	 * If mode == GMT_WRITE_SEGMENT then we combine segments into ONE segment per table.
	 */
	unsigned int hdr, fmt;
	uint64_t tbl, seg, row, col, tbl_out = 0, row_out = 0, seg_out = 0;
	char record[BUFSIZ] = {""};
	bool s_alloc, t_alloc, alloc, was;
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATATABLE *D = NULL;
	struct GMT_TEXTTABLE *T = NULL;
	struct GMT_DATASEGMENT *SD = NULL;
	struct GMT_TEXTSEGMENT *ST = NULL;
	s_alloc = t_alloc = alloc = (Out == NULL);
	if (alloc) {	/* Must allocate output textset */
		Out = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSET);
		Out->n_tables = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : In->n_tables;
		Out->table = gmt_M_memory (GMT, NULL, Out->n_tables, struct GMT_TEXTTABLE *);
	}
	was = GMT->current.setting.io_header[GMT_OUT];
	GMT->current.setting.io_header[GMT_OUT] = do_tbl_header (header);
	Out->n_segments = (mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : ((mode == GMT_WRITE_SEGMENT) ? In->n_tables : In->n_segments);
	Out->n_records  = In->n_records;
	for (tbl = 0; tbl < In->n_tables; tbl++) {
		if (mode == GMT_WRITE_SEGMENT) row_out = 0;	/* Reset row output counter on a per table basis */
		else if (mode == 0) seg_out = 0;	/* Reset segment output counter on a per table basis */
		if (alloc && (mode == 0 || mode == GMT_WRITE_SEGMENT)) s_alloc = true;	/* Need to allocate at least one segment per table */
		D = In->table[tbl];	/* Shorthand to current input data table */
		if (t_alloc) {
			Out->table[tbl_out] = T = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTTABLE);
			T->n_segments = T->n_alloc = (mode == GMT_WRITE_TABLE_SEGMENT || mode == GMT_WRITE_SEGMENT) ? 1 : ((mode == GMT_WRITE_TABLE) ? In->n_segments : D->n_segments);	/* Number of segments in this table */
			T->n_records  = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : D->n_records;	/* Number of data records int this table */
		}
		else T = Out->table[tbl_out];
		if (t_alloc) {
			if (D->n_headers && do_tbl_header(header)) {	/* Allocate and duplicate headers */
				T->n_headers = D->n_headers;	/* Same number of header records as input table */
				if (alloc) T->header = gmt_M_memory (GMT, NULL, T->n_headers, char *);
				for (hdr = 0; hdr < T->n_headers; hdr++) T->header[hdr] = strdup (D->header[hdr]);
			}
			T->segment = gmt_M_memory (GMT, NULL, T->n_segments, struct GMT_TEXTSEGMENT *);
		}
		for (seg = 0; seg < D->n_segments; seg++) {	/* For each input table segment */
			if (mode == 0 || mode == GMT_WRITE_TABLE) row_out = 0;	/* Reset row output counter on a per segment basis */
			SD = D->segment[seg];	/* Shorthand to current data segment */
			if (s_alloc) {	/* Allocate another segment */
				T->segment[seg_out] = gmt_M_memory (GMT, NULL, 1, struct GMT_TEXTSEGMENT);
				ST = T->segment[seg_out];	/* Shorthand to current text segment */
				ST->n_rows = ST->n_alloc = (mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : ((mode == GMT_WRITE_SEGMENT) ? D->n_records : SD->n_rows);
				ST->data = gmt_M_memory (GMT, NULL, ST->n_rows, char *);
				if (SD->header && do_seg_header(header)) ST->header = strdup (SD->header);
			}
			else
				ST = T->segment[seg_out];
			for (row = 0; row < SD->n_rows; row++, row_out++) {	/* Encode each row into a text string */
				fmt = 10;	/* Initialize record */
				for (col = 0; col < SD->n_columns; col++) {
					gmt_add_to_record (GMT, record, SD->data[col][row], col, GMT_IN, fmt);
					fmt = 1;	/* Prepend field separator */
				}
				ST->data[row_out] = strdup (record);
			}
			if (mode == GMT_WRITE_SEGMENT || mode == GMT_WRITE_TABLE_SEGMENT) s_alloc = false;	/* Only allocate this single segment, at least for this table */
			if (mode == 0 || mode == GMT_WRITE_TABLE) seg_out++;	/* More than one segment on output */
		}
		if (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) t_alloc = false;	/* Only allocate this single table */
		if (mode == 0 || mode == GMT_WRITE_SEGMENT) tbl_out++;	/* More than one segment on output */
	}
	GMT->current.setting.io_header[GMT_OUT] = was;
	return Out;
}

GMT_LOCAL void *api_dataset2matrix (struct GMTAPI_CTRL *API, struct GMT_DATASET *In, struct GMT_MATRIX *Out, unsigned int header, unsigned int mode) {
	/* Convert a dataset to a matrix.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * Header controls if segment headers are written as NaN recs
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 * If there are more than one segment we will insert NaN-records between segments.
	 */
	uint64_t tbl, seg, row, row_out, col, ij;
	bool alloc = (Out == NULL), add_NaN_record = (In->n_segments > 1 && do_seg_header(header));
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATATABLE *D = NULL;
	struct GMT_DATASEGMENT *SD = NULL;
	GMT_putfunction api_put_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;

	if (alloc) {	/* Must allocate the output matrix */
		if ((Out = gmtlib_create_matrix (GMT, 1U, GMT_OUT)) == NULL) return (NULL);
		Out->n_rows = In->n_records + (add_NaN_record ? In->n_segments : 0);
		Out->n_columns = Out->dim = In->n_columns;
		Out->type = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		if (gmtlib_alloc_univector (GMT, &(Out->data), Out->type, Out->n_rows * Out->n_columns)) {
			gmt_M_free (GMT, Out);
			return (NULL);
		}
		Out->alloc_mode = GMT_ALLOC_INTERNALLY;
	}
	api_put_val = api_select_put_function (API, Out->type);
	GMT_2D_to_index = api_get_2d_to_index (API, Out->shape, GMT_GRID_IS_REAL);
	for (tbl = row_out = 0; tbl < In->n_tables; tbl++) {
		D = In->table[tbl];	/* Shorthand to current input data table */
		for (seg = 0; seg < D->n_segments; seg++) {
			SD = D->segment[seg];	/* Shorthand */
			if (add_NaN_record) {
				for (col = 0; col < SD->n_columns; col++) {
					ij = GMT_2D_to_index (row_out, col, Out->dim);
					api_put_val (&(Out->data), ij, GMT->session.d_NaN);
				}
				row_out++;	/* Due to the extra NaN-data header record we just wrote */
			}
			for (row = 0; row < SD->n_rows; row++, row_out++) {
				for (col = 0; col < SD->n_columns; col++) {
					ij = GMT_2D_to_index (row_out, col, Out->dim);
					api_put_val (&(Out->data), ij, SD->data[col][row]);
				}
			}
		}
	}
	return Out;
}

GMT_LOCAL void *api_dataset2vector (struct GMTAPI_CTRL *API, struct GMT_DATASET *In, struct GMT_VECTOR *Out, unsigned int header, unsigned int mode) {
	/* Convert a dataset to vectors.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 * If there are more than one segment we will insert NaN-records between segments.
	 */
	uint64_t tbl, seg, row, row_out, col;
	bool alloc = (Out == NULL), add_NaN_record = (In->n_segments > 1 && do_seg_header(header));
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATATABLE *D = NULL;
	struct GMT_DATASEGMENT *SD = NULL;
	GMT_putfunction api_put_val = NULL;
	if (alloc) {
		if ((Out = gmt_create_vector (GMT, In->n_columns, GMT_OUT)) == NULL) return NULL;
		Out->n_rows = In->n_records + (add_NaN_record ? In->n_segments : 0);
		for (col = 0; col < Out->n_columns; col++)	/* Set same export data type for all vectors */
			Out->type[col] = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		if ((API->error = api_alloc_vectors (GMT, Out, Out->n_rows)) != GMT_OK) {
			gmt_M_free (GMT, Out);
			return (NULL);
		}
	}
	api_put_val = api_select_put_function (API, Out->type[0]);	/* Since all columns are of same type we get the pointer here */
	for (tbl = row_out = 0; tbl < In->n_tables; tbl++) {
		D = In->table[tbl];	/* Shorthand to current input data table */
		for (seg = 0; seg < D->n_segments; seg++) {
			SD = D->segment[seg];	/* Shorthand */
			if (add_NaN_record) {
				for (col = 0; col < SD->n_columns; col++)
					api_put_val (&(Out->data[col]), row_out, GMT->session.d_NaN);
				row_out++;	/* Due to the extra NaN-data header record we just wrote */
			}
			for (row = 0; row < SD->n_rows; row++, row_out++) {
				for (col = 0; col < SD->n_columns; col++)
					api_put_val (&(Out->data[col]), row_out, SD->data[col][row]);
			}
		}
	}
	return Out;
}

/* GMT_TEXTSET to GMT_* : */

GMT_LOCAL bool api_text2double_is_viable (struct GMT_CTRL *GMT, struct GMT_TEXTSET *In, bool strict) {
	/* Determine if we can convert text to data values.  Action depends on strict:
	 * strict = true: Determines if all records in the textset can be converted to floating point data.
	 * strict = false: Determines if two or more fields can be converted using first record only.
	 */
	bool viable = true;
	uint64_t tbl, seg, row, k;
	struct GMT_TEXTTABLE *T = NULL;
	struct GMT_TEXTSEGMENT *S = NULL;

	if (!strict) {
		unsigned int n_columns = gmtlib_conv_text2datarec (GMT, In->table[0]->segment[0]->data[0], GMT_BUFSIZ, GMT->current.io.curr_rec);
		return (n_columns > 1) ? true : false;
	}
	/* Do full strict check */
	for (tbl = 0; viable && tbl < In->n_tables; tbl++) {
		T = In->table[tbl];
		for (seg = 0; viable && seg < T->n_segments; seg++) {
			S = T->segment[seg];	/* Shorthand */
			for (row = 0; viable && row < S->n_rows; row++) {
				for (k = 0; viable && k < strlen (S->data[row]); k++) {
					if (!strncmp (&S->data[row][k], "NaN", 3U))
						k += 2;	/* Skip the NaN text */
					else if (!strchr ("0123456789.,; -+eE", S->data[row][k]))
						viable = false;
				}
			}
		}
	}
	return viable;
}

GMT_LOCAL void *api_textset2dataset (struct GMTAPI_CTRL *API, struct GMT_TEXTSET *In, struct GMT_DATASET *Out, unsigned int header, unsigned int dim, unsigned int mode) {
	/* Convert a textset to dataset.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * header controls what we do with headers.
	 * If dim > 0 then it is assumed to hold the number of columns, else we estimate it from first data record.
	 * If mode == GMT_WRITE_TABLE_SEGMENT then we combine all segments into a SINGLE segment in ONE table
	 * If mode == GMT_WRITE_TABLE then we collect all segments into ONE table.
	 * If mode == GMT_WRITE_SEGMENT then we combine segments into ONE segment per table.
	 * If header & GMT_STRICT_CONVERSION then we only do the conversion if possible 1:1, else return NULL.
	 * If header & GMT_LAX_CONVERSION then we do the conversion if at least the first two field convert, else return NULL.
	 */
	unsigned int hdr;
	uint64_t tbl, seg, row, col, n_rows, tbl_out = 0, row_out = 0, seg_out = 0;
	bool s_alloc, t_alloc, alloc, was;
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATATABLE *D = NULL;
	struct GMT_TEXTTABLE *T = NULL;
	struct GMT_DATASEGMENT *SD = NULL;
	struct GMT_TEXTSEGMENT *ST = NULL;

	if (header & GMT_STRICT_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, true);
		if (!viable) return NULL;	/* No can do */
		header -= (header & GMT_STRICT_CONVERSION);
	}
	else if (header & GMT_LAX_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, false);
		if (!viable) return NULL;	/* No can do */
		header -= (header & GMT_LAX_CONVERSION);
	}
	s_alloc = t_alloc = alloc = (Out == NULL);
	if (alloc) {
		if ((Out = gmt_M_memory (GMT, NULL, 1, struct GMT_DATASET)) == NULL) return_null (API, GMT_MEMORY_ERROR);
		Out->n_tables = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : In->n_tables;
		Out->table = gmt_M_memory (GMT, NULL, Out->n_tables, struct GMT_DATATABLE *);
		if (dim == 0) {	/* Must guess number of columns from first text record */
			Out->n_columns = gmtlib_conv_text2datarec (GMT, In->table[0]->segment[0]->data[0], GMT_BUFSIZ, GMT->current.io.curr_rec);
			GMT_Report (API, GMT_MSG_DEBUG, "api_textset2dataset: Determined that textset contains %d columns\n", Out->n_columns);
		}
		else
			Out->n_columns = dim;
	}
	was = GMT->current.setting.io_header[GMT_OUT];
	GMT->current.setting.io_header[GMT_OUT] = do_tbl_header (header);
	Out->n_segments = (mode == GMT_WRITE_TABLE_SEGMENT) ? 1 : ((mode == GMT_WRITE_SEGMENT) ? In->n_tables : In->n_segments);
	Out->n_records  = In->n_records;
	for (tbl = 0; tbl < In->n_tables; tbl++) {
		if (mode == GMT_WRITE_SEGMENT) row_out = 0;	/* Reset row output counter on a per table basis */
		else if (mode == 0) seg_out = 0;	/* Reset segment output counter on a per table basis */
		if (alloc && (mode == 0 || mode == GMT_WRITE_SEGMENT)) s_alloc = true;	/* Need to allocate at least one segment per table */
		T = In->table[tbl];
		if (t_alloc) {
			Out->table[tbl_out] = D = gmt_M_memory (GMT, NULL, 1, struct GMT_DATATABLE);
			D->n_columns = Out->n_columns;
		}
		else D = Out->table[tbl_out];
		D->n_segments = D->n_alloc = (mode == GMT_WRITE_TABLE_SEGMENT || mode == GMT_WRITE_SEGMENT) ? 1 : ((mode == GMT_WRITE_TABLE) ? In->n_segments : T->n_segments);	/* Number of segments in this table */
		D->n_records  = (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : T->n_records;	/* Number of data records int this table */
		if (t_alloc) {
			if (T->n_headers && do_tbl_header(header)) {
				D->n_headers = T->n_headers;	/* Same number of header records as input table */
				if (alloc) D->header = gmt_M_memory (GMT, NULL, D->n_headers, char *);
				for (hdr = 0; hdr < D->n_headers; hdr++) D->header[hdr] = strdup (T->header[hdr]);
			}
			D->segment = gmt_M_memory (GMT, NULL, D->n_segments, struct GMT_DATASEGMENT *);
		}
		for (seg = 0; seg < T->n_segments; seg++) {
			if (mode == 0 || mode == GMT_WRITE_TABLE) row_out = 0;	/* Reset row output counter on a per segment basis */
			ST = T->segment[seg];	/* Shorthand */
			if (s_alloc) {	/* Allocate this segment */
				n_rows = (mode == GMT_WRITE_TABLE_SEGMENT) ? In->n_records : ((mode == GMT_WRITE_SEGMENT) ? T->n_records : ST->n_rows);
				D->segment[seg_out] = SD = GMT_Alloc_Segment (API, GMT_IS_DATASET, n_rows, Out->n_columns, NULL, NULL);
				if (ST->header && do_seg_header(header)) SD->header = strdup (ST->header);
			}
			else
				SD = D->segment[seg_out];
			for (row = 0; row < ST->n_rows; row++, row_out++) {
				col = gmtlib_conv_text2datarec (GMT, ST->data[row], (unsigned int)Out->n_columns, GMT->current.io.curr_rec);
				for (col = 0; col < SD->n_columns; col++)
					SD->data[col][row_out] = GMT->current.io.curr_rec[col];
			}
			if (mode == GMT_WRITE_SEGMENT || mode == GMT_WRITE_TABLE_SEGMENT) s_alloc = false;	/* Only allocate this single segment, at least for this table */
			if (mode == 0 || mode == GMT_WRITE_TABLE) seg_out++;	/* More than one segment on output */
		}
		if (mode == GMT_WRITE_TABLE || mode == GMT_WRITE_TABLE_SEGMENT) t_alloc = false;	/* Only allocate this single table */
		if (mode == 0 || mode == GMT_WRITE_SEGMENT) tbl_out++;	/* More than one segment on output */
	}
	GMT->current.setting.io_header[GMT_OUT] = was;
	return Out;
}

GMT_LOCAL void *api_textset2matrix (struct GMTAPI_CTRL *API, struct GMT_TEXTSET *In, struct GMT_MATRIX *Out, unsigned int header, unsigned int dim, unsigned int mode) {
	/* Convert a textset to matrix.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * header controls what we do with headers.
	 * If dim > 0 then it is assumed to hold the number of columns, else we estimate it from first data record.
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 * If there are more than one segment we will insert NaN-records between segments.
	 * If header & GMT_STRICT_CONVERSION then we only do the conversion if possible 1:1, else return NULL.
	 * If header & GMT_LAX_CONVERSION then we do the conversion if at least the two first field convert, else return NULL.
	 */
	uint64_t tbl, seg, row, row_out, col, ij;
	bool alloc = (Out == NULL), add_NaN_record = (In->n_segments > 1 && do_seg_header(header));
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_TEXTTABLE *T = NULL;
	struct GMT_TEXTSEGMENT *ST = NULL;
	GMT_putfunction api_put_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;

	if (header & GMT_STRICT_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, true);
		if (!viable) return NULL;	/* No can do */
		mode -= (mode & GMT_STRICT_CONVERSION);
	}
	else if (header & GMT_LAX_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, false);
		if (!viable) return NULL;	/* No can do */
		mode -= (mode & GMT_LAX_CONVERSION);
	}

	if (alloc) {
		if ((Out = gmtlib_create_matrix (GMT, 1U, GMT_OUT)) == NULL) return_null (API, GMT_MEMORY_ERROR);
		if (dim == 0) {	/* Must guess number of columns from first text record */
			Out->n_columns = gmtlib_conv_text2datarec (GMT, In->table[0]->segment[0]->data[0], GMT_BUFSIZ, GMT->current.io.curr_rec);
			GMT_Report (API, GMT_MSG_DEBUG, "Determined that textset contains %d columns\n", Out->n_columns);
		}
		else
			Out->n_columns = dim;
		Out->type = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		Out->n_rows = In->n_records + (add_NaN_record ? In->n_segments : 0);
		if (gmtlib_alloc_univector (GMT, &(Out->data), Out->type, Out->n_rows * Out->n_columns)) {
			gmt_M_free (GMT, Out);
			return (NULL);
		}
		Out->alloc_mode = GMT_ALLOC_INTERNALLY;
	}
	api_put_val = api_select_put_function (API, Out->type);
	GMT_2D_to_index = api_get_2d_to_index (API, Out->shape, GMT_GRID_IS_REAL);
	for (tbl = row_out = 0; tbl < In->n_tables; tbl++) {
		T = In->table[tbl];
		for (seg = 0; seg < T->n_segments; seg++) {
			ST = In->table[tbl]->segment[seg];	/* Shorthand */
			if (add_NaN_record) {
				for (col = 0; col < Out->n_columns; col++) {
					ij = GMT_2D_to_index (row_out, col, Out->dim);
					api_put_val (&(Out->data), ij, GMT->session.d_NaN);
				}
				row_out++;	/* Due to the extra NaN-data header record we just wrote */
			}
			for (row = 0; row < ST->n_rows; row++, row_out++) {
				col = gmtlib_conv_text2datarec (GMT, ST->data[row], (unsigned int)Out->n_columns, GMT->current.io.curr_rec);
				for (col = 0; col < Out->n_columns; col++) {
					ij = GMT_2D_to_index (row_out, col, Out->dim);
					api_put_val (&(Out->data), ij, GMT->current.io.curr_rec[col]);
				}
			}
		}
	}
	return Out;
}

GMT_LOCAL void *api_textset2vector (struct GMTAPI_CTRL *API, struct GMT_TEXTSET *In, struct GMT_VECTOR *Out, unsigned int header, unsigned int dim, unsigned int mode) {
	/* Convert a textset to vectors.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * header controls what we do with headers.
	 * If dim > 0 then it is assumed to hold the number of columns, else we estimate it from first data record.
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 * If there are more than one segment we will insert NaN-records between segments.
	 * If header & GMT_STRICT_CONVERSION then we only do the conversion if possible 1:1, else return NULL.
	 * If header & GMT_LAX_CONVERSION then we do the conversion if at least one field converts, else return NULL.
	 */
	uint64_t tbl, seg, row, row_out, col;
	bool alloc = (Out == NULL), add_NaN_record = (In->n_segments > 1 && do_seg_header(header));
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_TEXTTABLE *T = NULL;
	struct GMT_TEXTSEGMENT *ST = NULL;
	GMT_putfunction api_put_val = NULL;

	if (header & GMT_STRICT_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, true);
		if (!viable) return NULL;	/* No can do */
	}
	else if (header & GMT_LAX_CONVERSION) {	/* Must ensure that all entries are numbers */
		bool viable = api_text2double_is_viable (GMT, In, false);
		if (!viable) return NULL;	/* No can do */
	}

	if (alloc) {
		if (dim == 0) {	/* Must guess number of columns from first text record */
			dim = gmtlib_conv_text2datarec (GMT, In->table[0]->segment[0]->data[0], GMT_BUFSIZ, GMT->current.io.curr_rec);
			GMT_Report (API, GMT_MSG_DEBUG, "Determined that textset contains %d columns\n", dim);
		}
		if ((Out = gmt_create_vector (GMT, dim, GMT_OUT)) == NULL)
			return_null (API, GMT_MEMORY_ERROR);
		Out->n_rows = In->n_records + (add_NaN_record ? In->n_segments : 0);
		for (col = 0; col < Out->n_columns; col++)	/* Set same export data type for all vectors */
			Out->type[col] = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		if ((API->error = api_alloc_vectors (GMT, Out, Out->n_rows)) != GMT_OK)
			return_null (API, GMT_MEMORY_ERROR);
	}
	api_put_val = api_select_put_function (API, Out->type[0]);	/* Since all columns are of same type we get the pointer here */
	for (tbl = row_out = 0; tbl < In->n_tables; tbl++) {
		T = In->table[tbl];
		for (seg = 0; seg < T->n_segments; seg++) {
			ST = T->segment[seg];	/* Shorthand */
			if (add_NaN_record) {
				for (col = 0; col < Out->n_columns; col++)
					api_put_val (&(Out->data[col]), row_out, GMT->session.d_NaN);
				row_out++;	/* Due to the extra NaN-data header record we just wrote */
			}
			for (row = 0; row < ST->n_rows; row++, row_out++) {
				(void)gmtlib_conv_text2datarec (GMT, ST->data[row], (unsigned int)Out->n_columns, GMT->current.io.curr_rec);
				for (col = 0; col < Out->n_columns; col++)
					api_put_val (&(Out->data[col]), row_out, GMT->current.io.curr_rec[col]);
			}
		}
	}
	return Out;
}

/* GMT_MATRIX to GMT_* : */

GMT_LOCAL void *api_matrix2dataset (struct GMTAPI_CTRL *API, struct GMT_MATRIX *In, struct GMT_DATASET *Out, unsigned int header) {
	/* Convert a matrix to a dataset (one table with one segment).
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 * header controls what we do with headers.
	 */
	uint64_t row, col, ij;
	bool alloc = (Out == NULL);
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATASEGMENT *SD = NULL;
	GMT_getfunction api_get_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_matrix2dataset: Header stripping not implemented yet - ignored!\n");
	if (alloc && (Out = gmtlib_create_dataset (GMT, 1U, 1U, In->n_rows, In->n_columns, GMT_IS_POINT, true)) == NULL)
		return_null (API, GMT_MEMORY_ERROR);
	SD = Out->table[0]->segment[0];	/* Shorthand to only segment in the dataset */
	api_get_val = api_select_get_function (API, In->type);
	GMT_2D_to_index = api_get_2d_to_index (API, In->shape, GMT_GRID_IS_REAL);
	for (row = 0; row < In->n_rows; row++) {
		for (col = 0; col < In->n_columns; col++) {
			ij = GMT_2D_to_index (row, col, In->dim);	/* Index into the user data matrix depends on layout (M->shape) */
			api_get_val (&(In->data), ij, &(SD->data[col][row]));
		}
	}
	return Out;
}

GMT_LOCAL void *api_matrix2textset (struct GMTAPI_CTRL *API, struct GMT_MATRIX *In, struct GMT_TEXTSET *Out, unsigned int header) {
	/* Convert a matrix to a textset (one table with one segment).
	 * If Out is not NULL then we assume it has enough rows to hold the textset records.
	 * header controls what we do with headers.
	 */
	uint64_t row, col, ij;
	unsigned int fmt;
	char record[BUFSIZ] = {""};
	bool alloc = (Out == NULL);
	double value;
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_TEXTSEGMENT *ST = NULL;
	GMT_getfunction api_get_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_matrix2textset: Header stripping not implemented yet - ignored!\n");
	if (alloc && (Out = gmtlib_create_textset (GMT, 1U, 1U, In->n_rows, false)) == NULL)
		return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */

	ST = Out->table[0]->segment[0];	/* Shorthand to only segment in the dataset */
	api_get_val = api_select_get_function (API, In->type);
	GMT_2D_to_index = api_get_2d_to_index (API, In->shape, GMT_GRID_IS_REAL);
	for (row = 0; row < In->n_rows; row++) {
		fmt = 10;	/* Initialize record */
		for (col = 0; col < In->n_columns; col++) {
			ij = GMT_2D_to_index (row, col, In->dim);	/* Index into the user data matrix depends on layout (M->shape) */
			api_get_val (&(In->data), ij, &value);
			gmt_add_to_record (GMT, record, value, col, GMT_IN, fmt);
			fmt = 1;	/* Prepend field separator */
		}
		ST->data[row] = strdup (record);
	}
	return Out;
}

GMT_LOCAL void *api_matrix2vector (struct GMTAPI_CTRL *API, struct GMT_MATRIX *In, struct GMT_VECTOR *Out, unsigned int header, unsigned int mode) {
	/* Convert a matrix to vectors.
	 * If Out is not NULL then we assume it has enough rows to hold the vector rows.
	 * header controls what we do with headers.
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 */
	uint64_t row, col, ij;
	bool alloc = (Out == NULL);
	double value;
	struct GMT_CTRL *GMT = API->GMT;
	GMT_getfunction api_get_val_m = NULL;
	GMT_putfunction api_put_val_v = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_matrix2vector: Header stripping not implemented yet - ignored!\n");
	if (alloc) {
		if ((Out = gmt_create_vector (GMT, In->n_columns, GMT_OUT)) == NULL)
			return_null (API, GMT_MEMORY_ERROR);
		Out->n_rows = In->n_rows;
		for (col = 0; col < Out->n_columns; col++)	/* Set same export data type for all vectors */
			Out->type[col] = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		if ((API->error = api_alloc_vectors (GMT, Out, Out->n_rows)) != GMT_OK) {
			gmt_M_free (GMT, Out);
			return_null (API, GMT_MEMORY_ERROR);
		}
	}

	api_get_val_m = api_select_get_function (API, In->type);
	api_put_val_v = api_select_put_function (API, GMT->current.setting.export_type);	/* Since all columns are of same type we get the pointer here */
	GMT_2D_to_index = api_get_2d_to_index (API, In->shape, GMT_GRID_IS_REAL);
	for (row = 0; row < In->n_rows; row++) {
		for (col = 0; col < In->n_columns; col++) {
			ij = GMT_2D_to_index (row, col, In->dim);	/* Index into the user data matrix depends on layout (M->shape) */
			api_get_val_m (&(In->data), ij, &value);
			api_put_val_v (&(Out->data[col]), row, value);
		}
	}
	return Out;
}

/* GMT_VECTOR to GMT_* : */

GMT_LOCAL void *api_vector2dataset (struct GMTAPI_CTRL *API, struct GMT_VECTOR *In, struct GMT_DATASET *Out, unsigned int header) {
	/* Convert a vector to a dataset (one table with one segment).
	 * header controls what we do with headers.
	 * If Out is not NULL then we assume it has enough rows and columns to hold the dataset records.
	 */
	uint64_t row, col;
	bool alloc = (Out == NULL);
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_DATASEGMENT *SD = NULL;
	GMT_getfunction api_get_val;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_vector2dataset: Header stripping not implemented yet - ignored!\n");
	if (alloc && (Out = gmtlib_create_dataset (GMT, 1U, 1U, In->n_rows, In->n_columns, GMT_IS_POINT, true)) == NULL)
		return_null (API, GMT_MEMORY_ERROR);
	SD = Out->table[0]->segment[0];	/* Shorthand to only segment in the dataset */
	for (col = 0; col < In->n_columns; col++) {
		api_get_val = api_select_get_function (API, In->type[col]);
		for (row = 0; row < In->n_rows; row++)
			api_get_val (&(In->data[col]), row, &(SD->data[col][row]));
	}
	return Out;
}

GMT_LOCAL void *api_vector2textset (struct GMTAPI_CTRL *API, struct GMT_VECTOR *In, struct GMT_TEXTSET *Out, unsigned int header) {
	/* Convert a vector to a textset (one table with one segment).
	 * header controls what we do with headers.
	 * If Out is not NULL then we assume it has enough rows to hold the textset records.
	 */
	uint64_t row, col;
	unsigned int fmt;
	char record[BUFSIZ] = {""};
	bool alloc = (Out == NULL);
	double value;
	struct GMT_CTRL *GMT = API->GMT;
	struct GMT_TEXTSEGMENT *ST = NULL;
	GMT_getfunction *api_get_val = NULL;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_vector2textset: Header stripping not implemented yet - ignored!\n");
	if (alloc && (Out = gmtlib_create_textset (GMT, 1U, 1U, In->n_rows, false)) == NULL)
		return_null (API, GMT_MEMORY_ERROR);	/* Allocation error */

	api_get_val = gmt_M_memory (GMT, NULL, In->n_columns, GMT_getfunction);
	for (col = 0; col < In->n_columns; col++)
		api_get_val[col] = api_select_get_function (API, In->type[col]);

	ST = Out->table[0]->segment[0];	/* Shorthand to only segment in the dataset */
	for (row = 0; row < In->n_rows; row++) {
		fmt = 10;	/* Initialize record */
		for (col = 0; col < In->n_columns; col++) {
			api_get_val[col] (&(In->data[col]), row, &value);
			gmt_add_to_record (GMT, record, value, col, GMT_IN, fmt);
			fmt = 1;	/* Prepend field separator */
		}
		ST->data[row] = strdup (record);
	}
	gmt_M_free (GMT, api_get_val);
	return Out;
}

GMT_LOCAL void *api_vector2matrix (struct GMTAPI_CTRL *API, struct GMT_VECTOR *In, struct GMT_MATRIX *Out, unsigned int header, unsigned int mode) {
	/* Convert a vector to a textset (one table with one segment).
	 * If Out is not NULL then we assume it has enough rows to hold the textset records.
	 * header controls what we do with headers.
	 * If mode > 0 then it is assumed to hold GMT_TYPE-1, else we assume the GMT default setting.
	 */
	uint64_t row, col, ij;
	bool alloc = (Out == NULL);
	double value;
	struct GMT_CTRL *GMT = API->GMT;
	GMT_getfunction api_get_val = NULL;
	GMT_putfunction api_put_val = NULL;
	p_func_uint64_t GMT_2D_to_index = NULL;
	if (header) GMT_Report (API, GMT_MSG_NORMAL, "api_vector2matrix: Header stripping not implemented yet - ignored!\n");
	if (alloc) {
		Out = gmtlib_create_matrix (GMT, 1U, GMT_OUT);
		Out->n_columns = In->n_columns;
		Out->n_rows = In->n_rows;
		Out->type = (mode) ? mode - 1 : API->GMT->current.setting.export_type;
		if (gmtlib_alloc_univector (GMT, &(Out->data), Out->type, Out->n_rows * Out->n_columns)) {
			gmt_M_free (GMT, Out);
			return (NULL);
		}
		Out->alloc_mode = GMT_ALLOC_INTERNALLY;
	}

	GMT_2D_to_index = api_get_2d_to_index (API, Out->shape, GMT_GRID_IS_REAL);
	api_put_val = api_select_put_function (API, Out->type);	/* Since all columns are of same type we get the pointer here */
	for (col = 0; col < In->n_columns; col++) {
		api_get_val = api_select_get_function (API, In->type[col]);
		for (row = 0; row < In->n_rows; row++) {
			api_get_val (&(In->data[col]), row, &value);
			ij = GMT_2D_to_index (row, col, Out->dim);
			api_put_val (&(Out->data), ij, value);
		}
	}
	return Out;
}

/* New function to convert between objects */

#define GMT_HEADER_MODE	0
#define GMT_COLUMN_MODE	1
#define GMT_TYPE_MODE	2
#define GMT_FORMAT_MODE	2	/* Same as GMT_TYPE_MODE [not a typo] */

void *GMT_Convert_Data (void *V_API, void *In, unsigned int family_in, void *Out, unsigned int family_out, unsigned int flag[]) {
	/* Convert between valid pairs of objects,  If Out == NULL then we allocate an output object,
	 * otherwise we assume we are given adequate space already.  This is most likely restricted to a GMT_MATRIX.
	 * flag is an array with three unsigned integers controlling various aspects of the conversion:
	 * flag[0]: Controls how headers are handled on output:
	 * 	 0 : All headers are passed on as is.  For Matrix/Vector all table headers are always ignored but
	 * 	     segment headers will be encoded as NaN records
	 * 	 1 : Headers are not copied, but segment headers are preserved
	 * 	 2 : Headers are preserved, but segment headers are initialized to blank
	 * 	 3 : All headers headers are eliminated
	 *	     The GMT Default settings in effect will control any output to files later.
	 * 	 Finally, if converting from TEXTSET to floating point representations, if the flag contains
	 *   GMT_STRICT_CONVERSION then we only do the conversion if it is possible, else return NULL.
	 *   If the flag contains GMT_LAX_CONVERSION then we do the conversion if at least one field converts, else return NULL.
	 * [Note if that happens it is not considered an error, so API->error is GMT_NOERROR].
	 * flag[1]: Controls how many columns to expect when converting TEXTSETS only.
	 *	0 : We determine number of columns by decoding the very first data record
	 *  >0: We use this value as the number of columns to decode and report.
	 * flag[2]: Controls the data type to use for MATRIX and VECTOR.
	 * 	0: Use the GMT default data type [GMT_EXPORT_TYPE]
	 * 	>0: Assumed to contain datatype + 1 (e.g., GMT_FLOAT+1, GMT_DOUBLE+1)
	 * If TEXTSET or DATASET, this integer controls the restructuring of the set:
	 * 	GMT_WRITE_TABLE_SEGMENT: Combine all segments into a SINGLE segment in ONE table
	 * 	GMT_WRITE_TABLE: Collect all segments into ONE table.
	 * 	GMT_WRITE_SEGMENT: Combine segments into ONE segment per table.
	 * 	0: Retain initial layout.
	 *
	 * The following conversions are valid; the brackets indicate any side-effects or limitations]
	 *
	 * DATASET -> TEXTSET, MATRIX,  VECTOR
	 * TEXTSET -> DATASET, MATRIX,  VECTOR [May result in NaNs; see GMT_STRICT_CONVERSION to limit conversion]
	 * MATRIX  -> DATASET, TEXTSET, VECTOR
	 * VECTOR  -> DATASET, TEXTSET, MATRIX
	 */
	int object_ID, item;
	bool may_fail = false;
	void *X = NULL;
	struct GMTAPI_CTRL *API = NULL;

	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	switch (family_in) {
		case GMT_IS_DATASET:
			switch (family_out) {
				case GMT_IS_DATASET:
					X = api_dataset2dataset (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_TYPE_MODE]);
					break;
				case GMT_IS_TEXTSET:
					X = api_dataset2textset (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_FORMAT_MODE]);
					break;
				case GMT_IS_MATRIX:
					X = api_dataset2matrix(API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_TYPE_MODE]);
					break;
				case GMT_IS_VECTOR:
					X = api_dataset2vector (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_TYPE_MODE]);
					break;
				default:
					API->error = GMT_NOT_A_VALID_FAMILY;
					break;
			}
			break;
		case GMT_IS_TEXTSET:
			may_fail = (flag[GMT_HEADER_MODE] & (GMT_STRICT_CONVERSION | GMT_LAX_CONVERSION));	/* Not an error if we "fail" to convert */
			switch (family_out) {
				case GMT_IS_DATASET:
					X = api_textset2dataset (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_COLUMN_MODE], flag[GMT_FORMAT_MODE]);
					break;
				case GMT_IS_MATRIX:
					X = api_textset2matrix (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_COLUMN_MODE], flag[GMT_TYPE_MODE]);
					break;
				case GMT_IS_VECTOR:
					X = api_textset2vector (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_COLUMN_MODE], flag[GMT_TYPE_MODE]);
					break;
				default:
					API->error = GMT_NOT_A_VALID_FAMILY;
					break;
			}
			break;
		case GMT_IS_MATRIX:
			switch (family_out) {
				case GMT_IS_DATASET:
					X = api_matrix2dataset (API, In, Out, flag[GMT_HEADER_MODE]);
					break;
				case GMT_IS_TEXTSET:
					X = api_matrix2textset (API, In, Out, flag[GMT_HEADER_MODE]);
					break;
				case GMT_IS_VECTOR:
					X = api_matrix2vector (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_TYPE_MODE]);
					break;
				default:
					API->error = GMT_NOT_A_VALID_FAMILY;
					break;
			}
			break;
		case GMT_IS_VECTOR:
			switch (family_out) {
				case GMT_IS_DATASET:
					X = api_vector2dataset (API, In, Out, flag[GMT_HEADER_MODE]);
					break;
				case GMT_IS_TEXTSET:
					X = api_vector2textset (API, In, Out, flag[GMT_HEADER_MODE]);
					break;
				case GMT_IS_MATRIX:
					X = api_vector2matrix (API, In, Out, flag[GMT_HEADER_MODE], flag[GMT_TYPE_MODE]);
					break;
				default:
					API->error = GMT_NOT_A_VALID_FAMILY;
					break;
			}
			break;
		default:
			API->error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	if (may_fail && X == NULL)
		return_null (API, GMT_NOERROR);
	if (API->error)
		return_null (API, API->error);
	if ((object_ID = GMT_Register_IO (API, family_out, GMT_IS_REFERENCE, GMT_IS_POINT, GMT_IN, NULL, X)) == GMT_NOTSET)
		return_null (API, API->error);	/* Failure to register */
	if ((item = gmtapi_validate_id (API, family_out, object_ID, GMT_IN, GMT_NOTSET)) == GMT_NOTSET)
		return_null (API, API->error);
	API->object[item]->data = X;		/* Retain pointer to the allocated data so we use garbage collection later */
#ifdef DEBUG
	api_list_objects (API, "GMT_Convert_Data");
#endif
	return (X);
}

#ifdef FORTRAN_API
void *GMT_Convert_Data_ (void *V_API, void *In, unsigned int *family_in, void *Out, unsigned int *family_out, unsigned int flag[]) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Convert_Data (GMT_FORTRAN, In, *family_in, Out, *family_out, flag));
}
#endif

struct GMT_DATASEGMENT *api_alloc_datasegment (void *V_API, uint64_t n_rows, uint64_t n_columns, char *header, struct GMT_DATASEGMENT *Sin) {
	/* Allocates space for a complete data segment and sets the segment header, if given.
	 * In Sin == NULL then we allocate a new segment; else we reallocate items of the existing segment */
	struct GMT_DATASEGMENT *S = NULL;
	struct GMTAPI_CTRL *API = NULL;
	bool first = true;
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if ((S = Sin) != NULL)	/* Existing segment given */
		first = false;
	else if ((S = gmt_M_memory (API->GMT, NULL, 1, struct GMT_DATASEGMENT)) == NULL) /* Something went wrong */
		return_null (V_API, GMT_MEMORY_ERROR);
	if (gmt_alloc_datasegment (API->GMT, S, n_rows, n_columns, first))  {	/* Something went wrong */
		if (first) gmt_M_free (API->GMT, S);
		return_null (V_API, GMT_MEMORY_ERROR);
	}
	if (header && strlen (header)) {	/* Gave a header string to (re)place in the segment */
		if (S->header) gmt_M_str_free (S->header);
		S->header = strdup (header);
	}
	return S;
}

GMT_LOCAL struct GMT_TEXTSEGMENT *api_alloc_textsegment (void *V_API, uint64_t n_rows, char *header, struct GMT_TEXTSEGMENT *Sin) {
	/* Allocates space for a complete text segment and sets the segment header, if given.
	 * In Sin == NULL then we allocate a new segment; else we reallocate items of the existing segment */
	struct GMT_TEXTSEGMENT *S = NULL;
	struct GMTAPI_CTRL *API = NULL;
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	if ((S = Sin) == NULL && (S = gmt_M_memory (API->GMT, NULL, 1, struct GMT_TEXTSEGMENT)) == NULL) /* Something went wrong */
		return_null (V_API, GMT_MEMORY_ERROR);
	if (gmt_alloc_textsegment (API->GMT, S, n_rows)) {	/* Something went wrong */
		gmt_M_free (API->GMT, S);
		return_null (V_API, GMT_MEMORY_ERROR);
	}
	if (header && strlen (header)) {	/* Gave a header string to (re)place in the segment */
		if (S->header) gmt_M_str_free (S->header);
		S->header = strdup (header);
	}
	return S;
}

void *GMT_Alloc_Segment (void *V_API, unsigned int family, uint64_t n_rows, uint64_t n_columns, char *header, void *S) {
	/* Deal with the two segment types or data and text.
	 * The n_columns is only used for data segments.
	 * header, if not NULL or blank, sets the segment header. */
	void *X = NULL;
	if (V_API == NULL) return_null (V_API, GMT_NOT_A_SESSION);
	switch (family) {	/* Only two supported families */
		case GMT_IS_DATASET:
			X = api_alloc_datasegment (V_API, n_rows, n_columns, header, S);
			break;
		case GMT_IS_TEXTSET:
			X = api_alloc_textsegment (V_API, n_rows, header, S);
			break;
		default:
			return_null (V_API, GMT_NOT_A_VALID_FAMILY);
			break;
	}
	return (X);
}

#ifdef FORTRAN_API
void *GMT_Alloc_Segment_ (void *V_API, unsigned int *family, uint64_t *n_rows, uint64_t *n_columns, char *header, void *S, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Set_Columns (GMT_FORTRAN, *family, *n_rows, *n_columns, header, S));
}
#endif

int GMT_Set_Columns (void *V_API, unsigned int n_cols, unsigned int mode) {
	/* Specify how many output columns to use for record-by-record output */
	int error = 0;
	uint64_t n_in;
	struct GMTAPI_CTRL *API = NULL;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;

	if ((n_in = gmt_get_cols (API->GMT, GMT_IN)) == 0) {	/* Get number of input columns */
		GMT_Report (API, GMT_MSG_NORMAL, "GMT_Set_Columns: Premature call - number of input columns not known yet\n");
		return_error (API, GMT_N_COLS_NOT_SET);
	}
	/* If no columns specified we set output to the same as input columns */
	if (n_cols == 0 && (error = gmt_set_cols (API->GMT, GMT_OUT, n_in)) != 0)
			return_error (API, GMT_N_COLS_NOT_SET);

	/* Get here when n_cols is not zero, so must consult mode */

	switch (mode) {
		case GMT_COL_FIX:	/* Specific a fixed number of columns */
			error = gmt_set_cols (API->GMT, GMT_OUT, n_cols);
			break;
		case GMT_COL_ADD:	/* Add to the number of input columns */
			error = gmt_set_cols (API->GMT, GMT_OUT, n_in + n_cols);
			break;
		case GMT_COL_SUB:	/* Subtract from the number of input columns */
			if (n_cols >= n_in) {
				GMT_Report (API, GMT_MSG_NORMAL, "GMT_Set_Columns: Cannot specify less than one output column!\n");
				return_error (API, GMT_DIM_TOO_SMALL);
			}
			error = gmt_set_cols (API->GMT, GMT_OUT, n_in - n_cols);
			break;
	}
	if (error) return_error (API, GMT_N_COLS_NOT_SET);
	return (GMT_NOERROR);
}

#ifdef FORTRAN_API
int GMT_Set_Columns_ (void *V_API, unsigned int *n_cols, unsigned int *mode) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Set_Columns (GMT_FORTRAN, *n_cols, *mode));
}
#endif

GMT_LOCAL int api_change_gridlayout (struct GMTAPI_CTRL *API, char *code, unsigned int mode, struct GMT_GRID *G, float *out) {
	enum GMT_enum_family family;
	unsigned int row, col, pad[4], old_layout, new_layout;
	uint64_t in_node, out_node;
	float *tmp = NULL;
	gmt_M_unused(mode);

	old_layout = api_decode_layout (API, G->header->mem_layout, &family);
	if (family != GMT_IS_GRID) return GMT_NOT_A_VALID_FAMILY;
	new_layout = api_decode_layout(API, code, &family);
	if (old_layout == new_layout) return GMT_NOERROR;	/* Nothing to do */

	/* Remove the high bits for complex data */
	old_layout &= 3;	new_layout &= 3;
	/* Grids may be column vs row oriented and from top or from bottom */
	gmt_M_memcpy (pad, G->header->pad, 4, unsigned int);	/* Remember the pad */
	if (((tmp = out) == NULL) && (tmp = gmt_M_memory_aligned (API->GMT, NULL, G->header->size, float)) == NULL)
		return (GMT_MEMORY_ERROR);		/* Something went wrong */

	gmt_grd_pad_off (API->GMT, G);	/* Simplify working with no pad */
	if (old_layout == 0 && new_layout == 2) { /* Change from TR to TC */
		for (row = 0, in_node = 0; row < G->header->n_rows; row++)
			for (col = 0; col < G->header->n_columns; col++, in_node++)
				tmp[(uint64_t)col * (uint64_t)G->header->n_rows + row] = G->data[in_node];
	}
	else if (old_layout == 0 && new_layout == 3) {	/* Change from TR to BC */
		for (row = 0, in_node = 0; row < G->header->n_rows; row++)
			for (col = 0; col < G->header->n_columns; col++, in_node++)
				tmp[(uint64_t)col * (uint64_t)G->header->n_rows + (G->header->n_rows - row - 1)] = G->data[in_node];
	}
	else if (old_layout == 2 && new_layout == 0) {	/* Change from TC to TR */
		for (row = 0, out_node = 0; row < G->header->n_rows; row++)
			for (col = 0; col < G->header->n_columns; col++, out_node++)
				tmp[out_node] = G->data[(uint64_t)col * (uint64_t)G->header->n_rows + row];
	}
	else if (old_layout == 3 && new_layout == 0) {	/* Change from BC to TR */
		for (row = 0, out_node = 0; row < G->header->n_rows; row++)
			for (col = 0; col < G->header->n_columns; col++, out_node++)
				tmp[out_node] = G->data[(uint64_t)col * (uint64_t)G->header->n_rows + (G->header->n_rows - row - 1)];
	}
	else {		/* Other cases to be added later ...*/
		GMT_Report (API, GMT_MSG_NORMAL, "api_change_gridlayout: reordering function for case %s -> %s not yet written. Doing nothing\n",
		            G->header->mem_layout, code);
		for (out_node = 0; out_node < G->header->size; out_node++)
			tmp[out_node] = G->data[out_node];
	}

	if (out == 0) {	/* Means we must update the grid data */
		gmt_M_free_aligned (API->GMT, G->data);			/* Free previous aligned grid memory */
		G->data = tmp;
	}
	gmt_grd_pad_on (API->GMT, G, pad);	/* Restore pad on grid */
	return (GMT_NOERROR);
}

GMT_LOCAL int api_change_imagelayout (struct GMTAPI_CTRL *API, char *code, unsigned int mode, struct GMT_IMAGE *I, unsigned char *out1, unsigned char *out2) {
	/* code: The new memory layout code
	   mode:
	   out1:  Array with the data converted to the new layout.
	   out2:  Array with the transparencies converted to the new layout.
	         If NULL on input the necessary memory is allocated in this function, otherwise
	         it is ASSUMED that it points a memory chunk big enough to hold the reshuffled data.
	*/
	unsigned char *tmp = NULL, *alpha = NULL;
	enum GMT_enum_family family;
	unsigned int old_layout, new_layout;
	uint64_t band, row, col, in_node, out_node;
	gmt_M_unused(mode);

	old_layout = api_decode_layout (API, I->header->mem_layout, &family);
	if (family != GMT_IS_IMAGE) return GMT_NOT_A_VALID_FAMILY;
	new_layout = api_decode_layout(API, code, &family);
	if (old_layout == new_layout) return GMT_NOERROR;	/* Nothing to do */

	/* Images may be column vs row oriented, from top or from bottom and may be Band|Line|Pixel interleaved
	   That sums up to a lot of combinations. We will add them on a by-need basis. */
	if ((tmp = out1) == NULL && (tmp = gmt_M_memory_aligned (API->GMT, NULL, I->header->n_bands * I->header->size, unsigned char)) == NULL)
		return (GMT_MEMORY_ERROR);		/* Something went wrong */
	if (I->alpha && (alpha = out2) == NULL && (alpha = gmt_M_memory_aligned (API->GMT, NULL, I->header->size, unsigned char)) == NULL) {
		if (out2 == NULL) gmt_M_free (API->GMT, alpha);
		gmt_M_free_aligned (API->GMT, tmp);
		return (GMT_MEMORY_ERROR);		/* Something went wrong */
	}

	if (old_layout == 8 && new_layout == 2) {	/* Change from TRP to TCB */
		out_node = 0;
		for (row = 0; row < I->header->n_rows; row++)
			for (col = 0; col < I->header->n_columns; col++)
				for (band = 0; band < 3; band++) {
					in_node = row + col*I->header->n_rows + band*I->header->nm;
					tmp[in_node] = (uint8_t)I->data[out_node++];
				}
	}
	//else if (old_layout == 2 && new_layout == 0) {}	/* Change from TCB to TRB */
	else {		/* Other cases to be added later ...*/
		GMT_Report (API, GMT_MSG_NORMAL, "api_change_imagelayout: reordering function for case %s -> %s not yet written. Doing nothing.\n",
		            I->header->mem_layout, code);
		for (out_node = 0; out_node < I->header->size; out_node++)
			tmp[out_node] = I->data[out_node];
	}

	if (out1 == 0) {	/* Means we must update the Image data */
		if (I->alloc_mode != GMT_ALLOC_EXTERNALLY)
			gmt_M_free_aligned (API->GMT, I->data);			/* Free previous aligned image memory */
		I->data = tmp;
	}
	if (out2 == 0 && alpha) {	/* Means we must update the alpha data */
		gmt_M_free_aligned (API->GMT, I->alpha);		/* Free previous aligned image transparency */
		I->alpha = alpha;
	}

	return (GMT_NOERROR);
}

int GMT_Change_Layout (void *V_API, unsigned int family, char *code, unsigned int mode, void *obj, void *out, void *alpha) {
	/* Reorder the memory layout of a grid or image given the new desired layout in code.
	 * If out == NULL then we allocate space to hold the new grid|image and replace obj->data with this new array.
	 *   For grids we preserve any padding in effect for the object but for out we have no padding.
	 * Otherwise we assume out points to allcoated memory and we simply fill it out, assuming no pad.
	 * mode is presently unused.
	 * alpha is only considered for images and may be used to return a modified transparency array.
	 */
	struct GMTAPI_CTRL *API = NULL;
	int error;
	if (V_API == NULL) return_error (V_API, GMT_NOT_A_SESSION);
	API = api_get_api_ptr (V_API);
	API->error = GMT_NOERROR;
	switch (family) {
		case GMT_IS_GRID:
			error = api_change_gridlayout (V_API, code, mode, obj, out);
			break;
		case GMT_IS_IMAGE:
			error = api_change_imagelayout (V_API, code, mode, obj, out, alpha);
			break;
		default:
			error = GMT_NOT_A_VALID_FAMILY;
			break;
	}
	return_error (API, error);
}

/* Deal with assignments of custom vectors and matrices to GMT containers */

int GMT_Put_Vector (void *API, struct GMT_VECTOR *V, unsigned int col, unsigned int type, void *vector) {
	/* Hooks a users custom vector onto V's column array and sets the type.
	 * It is the user's respondibility to pass correct type for the given vector. */
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	if (col >= V->n_columns) return_error (API, GMT_DIM_TOO_LARGE);
	switch (type) {
		case GMT_DOUBLE:	V->type[col] = GMT_DOUBLE;	V->data[col].f8  = vector;	break;
		case GMT_FLOAT:		V->type[col] = GMT_FLOAT;	V->data[col].f4  = vector;	break;
		case GMT_ULONG:		V->type[col] = GMT_ULONG;	V->data[col].ui8 = vector;	break;
		case GMT_LONG:		V->type[col] = GMT_LONG;	V->data[col].si8 = vector;	break;
		case GMT_UINT:		V->type[col] = GMT_UINT;	V->data[col].ui4 = vector;	break;
		case GMT_INT:		V->type[col] = GMT_INT;		V->data[col].si4 = vector;	break;
		case GMT_USHORT:	V->type[col] = GMT_USHORT;	V->data[col].ui2 = vector;	break;
		case GMT_SHORT:		V->type[col] = GMT_SHORT;	V->data[col].si2 = vector;	break;
		case GMT_UCHAR:		V->type[col] = GMT_UCHAR;	V->data[col].uc1 = vector;	break;
		case GMT_CHAR:		V->type[col] = GMT_CHAR;	V->data[col].sc1 = vector;	break;
		default:
			return_error (API, GMT_NOT_A_VALID_TYPE);
			break;
	}
	return GMT_NOERROR;
}

void *GMT_Get_Vector (void *API, struct GMT_VECTOR *V, unsigned int col) {
	/* Returns a pointer to the specified column array. Users can consult
	 * V->type[col] to know what data type is pointed to.  */
	void *vector = NULL;
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	if (col >= V->n_columns) return_null (API, GMT_DIM_TOO_LARGE);
	switch (V->type[col]) {
		case GMT_DOUBLE:	vector = V->data[col].f8;	break;
		case GMT_FLOAT:		vector = V->data[col].f4;	break;
		case GMT_ULONG:		vector = V->data[col].ui8;	break;
		case GMT_LONG:		vector = V->data[col].si8;	break;
		case GMT_UINT:		vector = V->data[col].ui4;	break;
		case GMT_INT:		vector = V->data[col].si4;	break;
		case GMT_USHORT:	vector = V->data[col].ui2;	break;
		case GMT_SHORT:		vector = V->data[col].si2;	break;
		case GMT_UCHAR:		vector = V->data[col].uc1;	break;
		case GMT_CHAR:		vector = V->data[col].sc1;	break;
		default:
			return_null (API, GMT_NOT_A_VALID_TYPE);
			break;
	}
	return vector;
}

int GMT_Put_Matrix (void *API, struct GMT_MATRIX *M, unsigned int type, void *matrix) {
	/* Hooks a user's custom matrix onto M's data array and sets the type.
	 * It is the user's respondibility to pass correct type for the given matrix. */
	if (API == NULL) return_error (API, GMT_NOT_A_SESSION);
	switch (type) {
		case GMT_DOUBLE:	M->type = GMT_DOUBLE;	M->data.f8  = matrix;	break;
		case GMT_FLOAT:		M->type = GMT_FLOAT;	M->data.f4  = matrix;	break;
		case GMT_ULONG:		M->type = GMT_ULONG;	M->data.ui8 = matrix;	break;
		case GMT_LONG:		M->type = GMT_LONG;		M->data.si8 = matrix;	break;
		case GMT_UINT:		M->type = GMT_UINT;		M->data.ui4 = matrix;	break;
		case GMT_INT:		M->type = GMT_INT;		M->data.si4 = matrix;	break;
		case GMT_USHORT:	M->type = GMT_USHORT;	M->data.ui2 = matrix;	break;
		case GMT_SHORT:		M->type = GMT_SHORT;	M->data.si2 = matrix;	break;
		case GMT_UCHAR:		M->type = GMT_UCHAR;	M->data.uc1 = matrix;	break;
		case GMT_CHAR:		M->type = GMT_CHAR;		M->data.sc1 = matrix;	break;
		default:
			return_error (API, GMT_NOT_A_VALID_TYPE);
			break;
	}
	return GMT_NOERROR;
}

void *GMT_Get_Matrix (void *API, struct GMT_MATRIX *M) {
	/* Returns a pointer to the matrix.  Users can consult
	 * M->type to know what data type is pointed to.  */
	void *matrix = NULL;
	if (API == NULL) return_null (API, GMT_NOT_A_SESSION);
	switch (M->type) {
		case GMT_DOUBLE:	matrix = M->data.f8;	break;
		case GMT_FLOAT:		matrix = M->data.f4;	break;
		case GMT_ULONG:		matrix = M->data.ui8;	break;
		case GMT_LONG:		matrix = M->data.si8;	break;
		case GMT_UINT:		matrix = M->data.ui4;	break;
		case GMT_INT:		matrix = M->data.si4;	break;
		case GMT_USHORT:	matrix = M->data.ui2;	break;
		case GMT_SHORT:		matrix = M->data.si2;	break;
		case GMT_UCHAR:		matrix = M->data.uc1;	break;
		case GMT_CHAR:		matrix = M->data.sc1;	break;
		default:
			return_null (API, GMT_NOT_A_VALID_TYPE);
			break;
	}
	return matrix;
}

/* Backwards compatibility for old API functions from 5.1-2 no longer in favor
 * as the Virtual File concept is much easier to understand and use. */

/*! . */
int GMT_Get_Value (void *V_API, const char *arg, double *par) {

	return (GMT_Get_Values (V_API, arg, par, 999));
}
#ifdef FORTRAN_API
int GMT_Get_Value_ (char *arg, double par[], int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_Values (GMT_FORTRAN, arg, par, len));
}
#endif

/*! . */
void *GMT_Retrieve_Data (void *API, int object_ID) {
	return api_retrieve_data (API, object_ID);
}

#ifdef FORTRAN_API
void *GMT_Retrieve_Data_ (int *object_ID) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (api_retrieve_data (GMT_FORTRAN, *object_ID));
}
#endif

void *GMT_Get_Data (void *API, int object_ID, unsigned int mode, void *data) {
	return api_get_data (API, object_ID, mode, data);
}

#ifdef FORTRAN_API
void *GMT_Get_Data_ (int *ID, int *mode, void *data) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (api_get_data (GMT_FORTRAN, *ID, *mode, data));
}
#endif

int GMT_Put_Data (void *API, int object_ID, unsigned int mode, void *data) {
	return api_put_data (API, object_ID, mode, data);
}

#ifdef FORTRAN_API
int GMT_Put_Data_ (int *object_ID, unsigned int *mode, void *data) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (api_put_data (GMT_FORTRAN, *object_ID, *mode, data));
}
#endif

int GMT_Encode_ID (void *API, char *filename, int object_ID) {
	return (api_encode_id (API, filename, object_ID));
}

#ifdef FORTRAN_API
int GMT_Encode_ID_ (char *filename, int *object_ID, int len) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Encode_ID (GMT_FORTRAN, filename, *object_ID));
}
#endif

int GMT_Get_ID (void *API, unsigned int family, unsigned int direction, void *resource) {
	return (api_get_id (API, family, direction, resource));
}

#ifdef FORTRAN_API
int GMT_Get_ID_ (unsigned int *family, unsigned int *direction, void *resource) {
	/* Fortran version: We pass the global GMT_FORTRAN structure */
	return (GMT_Get_ID (GMT_FORTRAN, *family, *direction, resource));
}
#endif
