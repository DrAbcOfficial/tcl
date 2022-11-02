// Tcl Abstract List test command: "lstring"
#include <string.h>
#include <limits.h>
#include "tcl.h"
#include "tclAbstractList.h"

/*
 * Forward references
 */

Tcl_Obj *myNewLStringObj(Tcl_WideInt start,
			 Tcl_WideInt length);
static void freeRep(Tcl_Obj* alObj);
static Tcl_Obj* my_LStringObjSetElem(Tcl_Interp *interp,
				     Tcl_Obj *listPtr,
				     Tcl_Size numIndcies,
				     Tcl_Obj *const indicies[],
				     Tcl_Obj *valueObj);
static void DupLStringRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr);
static Tcl_WideInt my_LStringObjLength(Tcl_Obj *lstringObjPtr);
static int my_LStringObjIndex(Tcl_Interp *interp,
			      Tcl_Obj *lstringObj,
			      Tcl_WideInt index,
			      Tcl_Obj **charObjPtr);
static int my_LStringObjRange(Tcl_Interp *interp, Tcl_Obj *lstringObj,
			      Tcl_WideInt fromIdx, Tcl_WideInt toIdx,
			      Tcl_Obj **newObjPtr);
static int my_LStringObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj,
			  Tcl_Obj **newObjPtr);
static int my_LStringReplace(Tcl_Interp *interp,
		      Tcl_Obj *listObj,
		      Tcl_WideInt first,
		      Tcl_WideInt numToDelete,
		      Tcl_WideInt numToInsert,
		      Tcl_Obj *const insertObjs[]);
static int my_LStringGetElements(Tcl_Interp *interp,
				 Tcl_Obj *listPtr,
				 int *objcptr,
				 Tcl_Obj ***objvptr);

/*
 * Internal Representation of an lstring type value
 */

typedef struct LString {
    char *string;          // NULL terminated utf-8 string
    Tcl_WideInt strlen;    // num bytes in string
    Tcl_WideInt allocated; // num bytes allocated
    Tcl_Obj**elements;     // elements array, allocated when GetElements is
			   // called
} LString;

/*
 * AbstractList definition of an lstring type
 */
static Tcl_AbstractListType lstringTypes[12] = {
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
/**/	NULL, /*default NULL,*/
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
/**/	NULL, /*default DupLStringRep,*/
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
/**/	NULL, /*default my_LStringObjLength,*/
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
/**/	NULL, /*default my_LStringObjIndex,*/
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
/**/	NULL, /*default my_LStringObjRange,*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
/**/	NULL, /*defaults my_LStringObjReverse,*/
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
/**/	NULL, /*default NULL / *my_LStringGetElements,*/
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
/**/	NULL, /*default freeRep,*/
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
/**/	NULL, /*toString*/
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
/**/	NULL, /*default my_LStringObjSetElem, / * use default update string */
	NULL, /*default my_LStringReplace*/
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
/**/	NULL, /*default my_LStringReplace*/
    },
    {
	TCL_ABSTRACTLIST_VERSION_1,
	"lstring",
	NULL,
	DupLStringRep,
	my_LStringObjLength,
	my_LStringObjIndex,
	my_LStringObjRange,/*ObjRange*/
	my_LStringObjReverse,
        my_LStringGetElements,
        freeRep,
	NULL /*toString*/,
	my_LStringObjSetElem, /* use default update string */
	my_LStringReplace
    }
};


/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjIndex --
 *
 *	Implements the AbstractList Index function for the lstring type.  The
 *	Index function returns the value at the index position given. Caller
 *	is resposible for freeing the Obj.
 *
 * Results:
 *	TCL_OK on success. Returns a new Obj, with a 0 refcount in the
 *	supplied charObjPtr location. Call has ownership of the Obj.
 *
 * Side effects:
 *	Obj allocated.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringObjIndex(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_WideInt index,
    Tcl_Obj **charObjPtr)
{
  LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);

  (void)interp;

  if (0 <= index && index < lstringRepPtr->strlen) {
      char cchar[2];
      cchar[0] = lstringRepPtr->string[index];
      cchar[1] = 0;
      *charObjPtr = Tcl_NewStringObj(cchar,1);
  } else {
      *charObjPtr = Tcl_NewObj();
  }

  return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjLength --
 *
 *	Implements the AbstractList Length function for the lstring type.
 *	The Length function returns the number of elements in the list.
 *
 * Results:
 *	WideInt number of elements in the list.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_WideInt
my_LStringObjLength(Tcl_Obj *lstringObjPtr)
{
    LString *lstringRepPtr = (LString *)Tcl_AbstractListGetConcreteRep(lstringObjPtr);
    return lstringRepPtr->strlen;
}


/*
 *----------------------------------------------------------------------
 *
 * DupLStringRep --
 *
 *	Replicates the internal representation of the src value, and storing
 *	it in the copy
 *
 * Results:
 *	void
 *
 * Side effects:
 *	Modifies the rep of the copyObj.
 *
 *----------------------------------------------------------------------
 */

static void
DupLStringRep(Tcl_Obj *srcPtr, Tcl_Obj *copyPtr)
{
  LString *srcLString = (LString*)Tcl_AbstractListGetConcreteRep(srcPtr);
  LString *copyLString = (LString*)Tcl_Alloc(sizeof(LString));

  memcpy(copyLString, srcLString, sizeof(LString));
  copyLString->string = (char*)Tcl_Alloc(srcLString->allocated);
  strcpy(copyLString->string, srcLString->string);
  copyLString->elements = NULL;
  Tcl_AbstractListSetConcreteRep(copyPtr,copyLString);

  return;
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjSetElem --
 *
 *	Replace the element value at the given (nested) index with the
 *	valueObj provided.  If the lstring obj is shared, a new list is
 *	created conntaining the modifed element.
 *
 * Results:
 *	The modifed lstring is returned, either new or original. If the
 *	index is invalid, NULL is returned, and an error is added to the
 *	interp, if provided.
 *
 * Side effects:
 *	A new obj may be created.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
my_LStringObjSetElem(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_Size numIndicies,
    Tcl_Obj *const indicies[],
    Tcl_Obj *valueObj)
{
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    int index;
    const char *newvalue;
    int status;
    Tcl_Obj *returnObj;

    if (numIndicies > 1) {
	Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("Multiple indicies not supported by lstring."));
	return NULL;
    }

    status = Tcl_GetIntForIndex(interp, indicies[0], lstringRepPtr->strlen, &index);
    if (status != TCL_OK) {
	return NULL;
    }

    returnObj = Tcl_IsShared(lstringObj) ? Tcl_DuplicateObj(lstringObj) : lstringObj;
    lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(returnObj);

    if (index < 0) {
	index = 0;
    }
    if (index >= lstringRepPtr->strlen) {
	index = lstringRepPtr->strlen;
	lstringRepPtr->strlen++;
	lstringRepPtr->string = (char*)Tcl_Realloc(lstringRepPtr->string, lstringRepPtr->strlen+1);
    }

    newvalue = Tcl_GetString(valueObj);
    lstringRepPtr->string[index] = newvalue[0];

    Tcl_InvalidateStringRep(returnObj);

    return returnObj;
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjRange --
 *
 *	Creates a new Obj with a slice of the src listPtr.
 *
 * Results:
 *	A new Obj is assigned to newObjPtr. Returns TCL_OK
 *
 * Side effects:
 *	A new Obj is created.
 *
 *----------------------------------------------------------------------
 */

static int my_LStringObjRange(
    Tcl_Interp *interp,
    Tcl_Obj *lstringObj,
    Tcl_WideInt fromIdx,
    Tcl_WideInt toIdx,
    Tcl_Obj **newObjPtr)
{
    Tcl_Obj *rangeObj;
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    LString *rangeRep;
    Tcl_WideInt len = toIdx - fromIdx + 1;

    if ((fromIdx < 0 || lstringRepPtr->strlen < fromIdx) ||
	(toIdx < 0 || lstringRepPtr->strlen < toIdx)) {
	Tcl_SetObjResult(interp,
	    Tcl_ObjPrintf("Range out of bounds "));
	return TCL_ERROR;
    }

    if (len <= 0) {
	// Return empty value;
	*newObjPtr = Tcl_NewObj();
    } else {
	rangeRep = (LString*)Tcl_Alloc(sizeof(LString));
	rangeRep->allocated = len+1;
	rangeRep->strlen = len;
	rangeRep->string = (char*)Tcl_Alloc(rangeRep->allocated);
	strncpy(rangeRep->string,&lstringRepPtr->string[fromIdx],len);
	rangeRep->string[len] = 0;
	rangeRep->elements = NULL;
	rangeObj = Tcl_AbstractListObjNew(interp, Tcl_AbstractListGetType(lstringObj));
	Tcl_AbstractListSetConcreteRep(rangeObj, rangeRep);
	*newObjPtr = rangeObj;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringObjReverse --
 *
 *	Creates a new Obj with the the order of the elements in the lstring
 *	value reversed, where first is last and last is first, etc.
 *
 * Results:
 *	A new Obj is assigned to newObjPtr. Returns TCL_OK
 *
 * Side effects:
 *	A new Obj is created.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringObjReverse(Tcl_Interp *interp, Tcl_Obj *srcObj, Tcl_Obj **newObjPtr)
{
    LString *srcRep = (LString*)Tcl_AbstractListGetConcreteRep(srcObj);
    Tcl_Obj *revObj;
    LString *revRep = (LString*)Tcl_Alloc(sizeof(LString));
    Tcl_WideInt len;
    char *srcp, *dstp, *endp;
    len = srcRep->strlen;
    revRep->strlen = len;
    revRep->allocated = len+1;
    revRep->string = (char*)Tcl_Alloc(revRep->allocated);
    revRep->elements = NULL;
    srcp = srcRep->string;
    endp = &srcRep->string[len];
    dstp = &revRep->string[len];
    *dstp-- = 0;
    while (srcp < endp) {
	*dstp-- = *srcp++;
    }
    revObj = Tcl_AbstractListObjNew(interp, Tcl_AbstractListGetType(srcObj));
    Tcl_AbstractListSetConcreteRep(revObj, revRep);
    *newObjPtr = revObj;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringReplace --
 *
 *	Delete and/or Insert elements in the list, starting at index first.
 *	See more details in the comments below. This should not be called with
 *	a Shared Obj.
 *
 * Results:
 *	The value of the listObj is modified.
 *
 * Side effects:
 *	The string rep is invalidated.
 *
 *----------------------------------------------------------------------
 */

static int
my_LStringReplace(
    Tcl_Interp *interp,
    Tcl_Obj *listObj,
    Tcl_WideInt first,
    Tcl_WideInt numToDelete,
    Tcl_WideInt numToInsert,
    Tcl_Obj *const insertObjs[])
{
    LString *lstringRep = (LString*)Tcl_AbstractListGetConcreteRep(listObj);
    Tcl_WideInt newLen;
    Tcl_WideInt x, ix, kx;
    char *newStr;
    char *oldStr = lstringRep->string;
    (void)interp;

    if (numToDelete < 0) numToDelete = 0;
    if (numToInsert < 0) numToInsert = 0;

    newLen = lstringRep->strlen - numToDelete + numToInsert;

    if (newLen >= lstringRep->allocated) {
	lstringRep->allocated = newLen+1;
	newStr = Tcl_Alloc(lstringRep->allocated);
	newStr[newLen] = 0;
    } else {
	newStr = oldStr;
    }

    /* Tcl_ListObjReplace replaces zero or more elements of the list
     * referenced by listPtr with the objc values in the array referenced by
     * objv.
     *
     * If listPtr does not point to a list value, Tcl_ListObjReplace
     * will attempt to convert it to one; if the conversion fails, it returns
     * TCL_ERROR and leaves an error message in the interpreter's result value
     * if interp is not NULL. Otherwise, it returns TCL_OK after replacing the
     * values.
     *
     *    * If objv is NULL, no new elements are added.
     *
     *    * If the argument first is zero or negative, it refers to the first
     *      element.
     *
     *    * If first is greater than or equal to the number of elements in the
     *      list, then no elements are deleted; the new elements are appended
     *      to the list. count gives the number of elements to replace.
     *
     *    * If count is zero or negative then no elements are deleted; the new
     *      elements are simply inserted before the one designated by first.
     *      Tcl_ListObjReplace invalidates listPtr's old string representation.
     *
     *    * The reference counts of any elements inserted from objv are
     *      incremented since the resulting list now refers to them. Similarly,
     *      the reference counts for any replaced values are decremented.
     */

    // copy 0 to first-1
    if (newStr != oldStr) {
	strncpy(newStr, oldStr, first);
    }

    // move front elements to keep
    for(x=0, kx=0; x<newLen && kx<first; kx++, x++) {
	newStr[x] = oldStr[kx];
    }
    // Insert new elements into new string
    for(x=first, ix=0; ix<numToInsert; x++, ix++) {
	char const *svalue = Tcl_GetString(insertObjs[ix]);
	newStr[x] = svalue[0];
    }
    // Move remaining elements
    if ((first+numToDelete) < newLen) {
	for(/*x,*/ kx=first+numToDelete; (kx <lstringRep->strlen && x<newLen); x++, kx++) {
	    newStr[x] = oldStr[kx];
	}
    }

    // Terminate new string.
    newStr[newLen] = 0;


    if (oldStr != newStr) {
	Tcl_Free(oldStr);
    }
    lstringRep->string = newStr;
    lstringRep->strlen = newLen;

    /* Changes made to value, string rep no longer valid */
    Tcl_InvalidateStringRep(listObj);

    return TCL_OK;
}

static Tcl_AbstractListType *
my_SetAbstractProc(Tcl_AbstractListProcType ptype)
{
    Tcl_AbstractListType *typePtr = &lstringTypes[11];
    if (TCL_ABSL_NEW <= ptype && ptype <= TCL_ABSL_REPLACE) {
	typePtr = &lstringTypes[ptype];
    }
    return typePtr;
}


/*
 *----------------------------------------------------------------------
 *
 * my_NewLStringObj --
 *
 *	Creates a new lstring Obj using the string value of objv[0]
 *
 * Results:
 *	results
 *
 * Side effects:
 *	side effects
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
my_NewLStringObj(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
    LString *lstringRepPtr;
    size_t repSize;
    Tcl_Obj *lstringPtr;
    const char *string;
    static const char* procTypeNames[] = {
	"NEW", "DUPREP", "LENGTH", "INDEX",
	"SLICE", "REVERSE", "GETELEMENTS", "FREEREP",
	"TOSTRING", "SETELEMENT", "REPLACE", NULL
    };
    int i = 0;
    Tcl_AbstractListProcType ptype;
    Tcl_AbstractListType *lstringTypePtr = &lstringTypes[11];

    repSize = sizeof(LString);
    lstringRepPtr = (LString*)Tcl_Alloc(repSize);

    while (i<objc) {
	const char *s = Tcl_GetString(objv[i]);
	if (strcmp(s, "-not")==0) {
	    i++;
	    if (Tcl_GetIndexFromObj(interp, objv[i], procTypeNames, "proctype", 0, &ptype)==TCL_OK) {
		lstringTypePtr = my_SetAbstractProc(ptype);
	    }
	} else if (strcmp(s, "--") == 0) {
	    // End of options
	    i++;
	    break;
	} else {
	    break;
	}
	i++;
    }
    if (i != objc-1) {
	Tcl_WrongNumArgs(interp, 0, objv, "lstring string");
	return NULL;
    }
    string = Tcl_GetString(objv[i]);

    lstringPtr = Tcl_AbstractListObjNew(interp, lstringTypePtr);
    lstringRepPtr->strlen = strlen(string);
    lstringRepPtr->allocated = lstringRepPtr->strlen + 1;
    lstringRepPtr->string = (char*)Tcl_Alloc(lstringRepPtr->allocated);
    strcpy(lstringRepPtr->string, string);
    lstringRepPtr->elements = NULL;
    Tcl_AbstractListSetConcreteRep(lstringPtr, lstringRepPtr);
    if (lstringRepPtr->strlen > 0) {
	Tcl_InvalidateStringRep(lstringPtr);
    } else {
	Tcl_InitStringRep(lstringPtr, NULL, 0);
    }

    return lstringPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * freeRep --
 *
 *	Free the value storage of the lstring Obj.
 *
 * Results:
 *	void
 *
 * Side effects:
 *	Memory free'd.
 *
 *----------------------------------------------------------------------
 */

static void
freeRep(Tcl_Obj* lstringObj)
{
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    if (lstringRepPtr->string) {
	Tcl_Free(lstringRepPtr->string);
    }
    if (lstringRepPtr->elements) {
	Tcl_Obj **objptr = lstringRepPtr->elements;
	while (objptr < &lstringRepPtr->elements[lstringRepPtr->strlen]) {
	    Tcl_DecrRefCount(*objptr++);
	}
	Tcl_Free((char*)lstringRepPtr->elements);
	lstringRepPtr->elements = NULL;
    }
    Tcl_Free((char*)lstringRepPtr);
    Tcl_AbstractListSetConcreteRep(lstringObj, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * my_LStringGetElements --
 *
 *	Get the elements of the list in an array.
 *
 * Results:
 *	objc, objv return values
 *
 * Side effects:
 *	A Tcl_Obj is stored for every element of the abstract list
 *
 *----------------------------------------------------------------------
 */

static int my_LStringGetElements(Tcl_Interp *interp,
				 Tcl_Obj *lstringObj,
				 int *objcptr,
				 Tcl_Obj ***objvptr)
{
    LString *lstringRepPtr = (LString*)Tcl_AbstractListGetConcreteRep(lstringObj);
    Tcl_Obj **objPtr;
    char *cptr = lstringRepPtr->string;
    (void)interp;
    if (lstringRepPtr->strlen == 0) {
	*objcptr = 0;
	*objvptr = NULL;
	return TCL_OK;
    }
    if (lstringRepPtr->elements == NULL) {
	lstringRepPtr->elements = (Tcl_Obj**)Tcl_Alloc(sizeof(Tcl_Obj*) * lstringRepPtr->strlen);
	objPtr=lstringRepPtr->elements;
	while (objPtr<&lstringRepPtr->elements[lstringRepPtr->strlen]) {
	    *objPtr = Tcl_NewStringObj(cptr++,1);
	    Tcl_IncrRefCount(*objPtr++);
	}
    }
    *objvptr = lstringRepPtr->elements;
    *objcptr = lstringRepPtr->strlen;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * lLStringObjCmd --
 *
 *	Script level command that creats an lstring Obj value.
 *
 * Results:
 *	Returns and lstring Obj value in the interp results.
 *
 * Side effects:
 *	Interp results modified.
 *
 *----------------------------------------------------------------------
 */

static int
lLStringObjCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj * const objv[])
{
  Tcl_Obj *lstringObj;

  (void)clientData;
  if (objc < 2) {
      Tcl_WrongNumArgs(interp, 1, objv, "string");
      return TCL_ERROR;
  }

  lstringObj = my_NewLStringObj(interp, objc-1, &objv[1]);

  if (lstringObj) {
      Tcl_SetObjResult(interp, lstringObj);
      return TCL_OK;
  }
  return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Lstring_Init --
 *
 *	DL load init function. Defines the "lstring" command.
 *
 * Results:
 *	"lstring" command added to the interp.
 *
 * Side effects:
 *	A new command is defined.
 *
 *----------------------------------------------------------------------
 */

int Tcl_ABSListTest_Init(Tcl_Interp *interp) {
    if (Tcl_InitStubs(interp, "8.7", 0) == NULL) {
	return TCL_ERROR;
    }
    Tcl_CreateObjCommand(interp, "lstring", lLStringObjCmd, NULL, NULL);
    Tcl_PkgProvide(interp, "abstractlisttest", "1.0.0");
    return TCL_OK;
}