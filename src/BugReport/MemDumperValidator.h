/*----------------------------------------------------------------------
Debugging Applications for Microsoft .NET and Microsoft Windows
Copyright ?1997-2003 John Robbins -- All rights reserved.
----------------------------------------------------------------------*/

#ifndef _MEMDUMPERVALIDATOR_H
#define _MEMDUMPERVALIDATOR_H

// Don't include this file directly; include BUGSLAYER.H
// instead.
#ifndef _BUGSLAYERUTIL_H
#error "Include BUGSLAYERUTIL.H instead of this file directly!"
#endif  // _BUGSLAYERUTIL_H

#ifdef __cplusplus
extern "C" {
#endif      // __cplusplus

// This library can be used only in _debug builds.
#ifdef _DEBUG

////////////////////////////////////////////////////////////////////////
// The typedefs for the dumper and validator functions
////////////////////////////////////////////////////////////////////////
// The memory dumper function. This function's only parameter is a
// pointer to the block of memory. This function can output the memory
// data for the block in any way it chooses, but to be consistent, it
// should use the same reporting mechanism the rest of the DCRT
// library uses.
typedef void (*PFNMEMDUMPER)(const void *) ;
// The validator function. This function's first parameter is the memory
// block to validate, and the second parameter is the context
// information passed to the ValidateAllBlocks function.
typedef void (*PFNMEMVALIDATOR)(const void * , const void *) ;

////////////////////////////////////////////////////////////////////////
// Useful macros
////////////////////////////////////////////////////////////////////////
// The macro used to set a Client block subtype value. Using this macro
// is the only approved means of setting the value of the dwValue field
// in the BSMDVINFO structure below.
#define CLIENT_BLOCK_VALUE(x) (_CLIENT_BLOCK|(x<<16))
// A macro to pick out the subtype
#define CLIENT_BLOCK_SUBTYPE(x) ((x >> 16) & 0xFFFF)

////////////////////////////////////////////////////////////////////////
// The header used to initialize the dumper and validator for a specific
// Client block subtype
////////////////////////////////////////////////////////////////////////
typedef struct tag_BSMDVINFO
{
    // The subtype value for the Client blocks. This value must be set
    // with the CLIENT_BLOCK_VALUE macro above. See the AddClientDV
    // function to find out how to have the extension assign this
    // number.
    unsigned long   dwValue      ;
    // The pointer to the dumper function
    PFNMEMDUMPER    pfnDump     ;
    // The pointer to the dumper function
    PFNMEMVALIDATOR pfnValidate ;
} BSMDVINFO , * LPBSMDVINFO ;

/*----------------------------------------------------------------------
FUNCTION        :   AddClientDV
DISCUSSION      :
    Adds a Client block dumper and validator to the list. If the
dwValue field in the BSMDVINFO structure is 0, the next value in
the list is assigned. The value returned must always be passed to
 _malloc_dbg as the subtype value of the Client block.
    If the subtype value is set with CLIENT_BLOCK_VALUE, a macro can be
used for the value passed to _malloc_dbg.
    Notice that there's no corresponding remove function. Why run the
risk of introducing bugs in debugging code?  Performance is a nonissue
when it comes to finding errors.
PARAMETERS      :
    lpBSMDVINFO - The pointer to the BSMDVINFO structure
RETURNS         :
    1 - The client block dumper and validator were properly added.
    0 - The client block dumper and validator couldn't be added.
----------------------------------------------------------------------*/
    BUGSUTIL_DLLINTERFACE int __stdcall
            AddClientDV ( LPBSMDVINFO lpBSMDVINFO ) ;

/*----------------------------------------------------------------------
FUNCTION        :   ValidateAllBlocks
DISCUSSION      :
    Checks all the memory allocated out of the local heap. Also goes
through all Client blocks and calls the special validator function for
the different subtypes.
    It's probably best to call this function with the VALIDATEALLBLOCKS
macro below.
PARAMETERS      :
    pContext - The context information that will be passed to each
               validator function
RETURNS         :
    None
----------------------------------------------------------------------*/
    BUGSUTIL_DLLINTERFACE void __stdcall
                                 ValidateAllBlocks ( void * pContext ) ;

#ifdef __cplusplus
////////////////////////////////////////////////////////////////////////
// Helper C++ class macros
////////////////////////////////////////////////////////////////////////
// Declare this macro in your class just as you would an MFC macro.
#define DECLARE_MEMDEBUG(classname)                                 \
public   :                                                          \
    static BSMDVINFO  m_stBSMDVINFO ;                               \
    static void ClassDumper ( const void * pData ) ;                \
    static void ClassValidator ( const void * pData ,               \
                                     const void * pContext )       ;\
    static void * operator new ( size_t nSize )                     \
    {                                                               \
        if ( 0 == m_stBSMDVINFO.dwValue )                           \
        {                                                           \
            m_stBSMDVINFO.pfnDump     = classname::ClassDumper ;    \
            m_stBSMDVINFO.pfnValidate = classname::ClassValidator ; \
            AddClientDV ( &m_stBSMDVINFO ) ;                        \
        }                                                           \
        return ( _malloc_dbg ( nSize                   ,            \
                               (int)m_stBSMDVINFO.dwValue ,         \
                               __FILE__                ,            \
                               __LINE__                 ) ) ;       \
    }                                                               \
    static void * operator new ( size_t nSize        ,              \
                                 char * lpszFileName ,              \
                                 int    nLine         )             \
    {                                                               \
        if ( 0 == m_stBSMDVINFO.dwValue )                           \
        {                                                           \
            m_stBSMDVINFO.pfnDump     = classname::ClassDumper ;    \
            m_stBSMDVINFO.pfnValidate = classname::ClassValidator ; \
            AddClientDV ( &m_stBSMDVINFO ) ;                        \
        }                                                           \
        return ( _malloc_dbg ( nSize                   ,            \
                               (int)m_stBSMDVINFO.dwValue ,         \
                               lpszFileName            ,            \
                               nLine                    ) ) ;       \
    }                                                               \
    static void * operator new ( size_t nSize          ,            \
                                 int    /*iBlockType*/ ,            \
                                 char * lpszFileName   ,            \
                                 int    nLine           )           \
    {                                                               \
        if ( 0 == m_stBSMDVINFO.dwValue )                           \
        {                                                           \
            m_stBSMDVINFO.pfnDump     = classname::ClassDumper ;    \
            m_stBSMDVINFO.pfnValidate = classname::ClassValidator ; \
            AddClientDV ( &m_stBSMDVINFO ) ;                        \
        }                                                           \
        return ( _malloc_dbg ( nSize                   ,            \
                               (int)m_stBSMDVINFO.dwValue ,         \
                               lpszFileName            ,            \
                               nLine                    ) ) ;       \
    }                                                               \
    static void operator delete ( void * pData )                    \
    {                                                               \
        _free_dbg ( pData , (int)m_stBSMDVINFO.dwValue ) ;          \
    }                                                               \
    static void __cdecl operator delete ( void * _P ,               \
                                          int       ,               \
                                          char *    ,               \
                                          int        )              \
    {                                                               \
        ::operator delete ( _P ) ;                                  \
    }                                                               \
    static void __cdecl operator delete[] ( void * _P ,             \
                                            int       ,             \
                                            char *    ,             \
                                            int        )            \
    {                                                               \
        ::operator delete[]( _P ) ;                                 \
    }
    
// Declare this macro at the top of your CPP file.
#define IMPLEMENT_MEMDEBUG(classname)                               \
    BSMDVINFO  classname::m_stBSMDVINFO = { 0 , 0 , 0 }

// The macro for memory debugging allocations. If DEBUG_NEW is defined,
// it can be used.
#ifdef DEBUG_NEW
#define MEMDEBUG_NEW DEBUG_NEW
#else
#define MEMDEBUG_NEW new ( __FILE__ , __LINE__ )
#endif

#endif      // __cplusplus defined

////////////////////////////////////////////////////////////////////////
// Helper C macros
////////////////////////////////////////////////////////////////////////

// Use this macro for C-style allocations. The only problem
// with C is that you need to drag around a BSMDVINFO structure.
#define INITIALIZE_MEMDEBUG(lpBSMDVINFO , pfnD , pfnV )                \
    {                                                                  \
        ASSERT ( FALSE == IsBadWritePtr ( lpBSMDVINFO ,                \
                                          sizeof ( BSMDVINFO ) ) ) ;   \
        ((LPBSMDVINFO)lpBSMDVINFO)->dwValue = 0 ;                      \
        ((LPBSMDVINFO)lpBSMDVINFO)->pfnDump = pfnD ;                   \
        ((LPBSMDVINFO)lpBSMDVINFO)->pfnValidate = pfnV ;               \
        AddClientDV ( lpBSMDVINFO ) ;                                  \
    }

// The macros that map the C-style allocations. It might be easier if
// you use macros to wrap these so that you don't have to remember which
// BSMDVINFO block value to drag around with each memory usage function.
#define MEMDEBUG_MALLOC(lpBSMDVINFO , nSize)                    \
            _malloc_dbg ( nSize            ,                    \
                          ((LPBSMDVINFO)lpBSMDVINFO)->dwValue , \
                          __FILE__         ,                    \
                          __LINE__          )
#define MEMDEBUG_REALLOC(lpBSMDVINFO , pBlock , nSize)          \
            _realloc_dbg( pBlock             ,                  \
                          nSize              ,                  \
                          ((LPBSMDVINFO)lpBSMDVINFO)->dwValue  ,\
                          __FILE__           ,                  \
                          __LINE__           )
#define MEMDEBUG_EXPAND(lpBSMDVINFO , pBlock , nSize )          \
            _expand_dbg( pBlock             ,                   \
                         nSize              ,                   \
                         ((LPBSMDVINFO)lpBSMDVINFO)->dwValue  , \
                         __FILE__           ,                   \
                         __LINE__            )
#define MEMDEBUG_FREE(lpBSMDVINFO , pBlock)                     \
            _free_dbg ( pBlock            ,                     \
                        ((LPBSMDVINFO)lpBSMDVINFO)->dwValue )
#define MEMDEBUG_MSIZE(lpBSMDVINFO , pBlock)                    \
            _msize_dbg ( pBlock , ((LPBSMDVINFO)lpBSMDVINFO)->dwValue )

// Macro to call ValidateAllBlocks
#define VALIDATEALLBLOCKS(x)   ValidateAllBlocks ( x )

#else       // _DEBUG is not defined.

#ifdef __cplusplus
#define DECLARE_MEMDEBUG(classname)
#define IMPLEMENT_MEMDEBUG(classname)
#define MEMDEBUG_NEW new
#endif      // __cplusplus

#define INITIALIZE_MEMDEBUG(lpBSMDVINFO , pfnD , pfnV )

#define MEMDEBUG_MALLOC(lpBSMDVINFO , nSize) \
                    malloc ( nSize )
#define MEMDEBUG_REALLOC(lpBSMDVINFO , pBlock , nSize) \
                    realloc ( pBlock , nSize )
#define MEMDEBUG_EXPAND(lpBSMDVINFO , pBlock , nSize)  \
                    _expand ( pBlock , nSize )
#define MEMDEBUG_FREE(lpBSMDVINFO , pBlock) \
                    free ( pBlock )
#define MEMDEBUG_MSIZE(lpBSMDVINFO , pBlock) \
                    _msize ( pBlock )

#define VALIDATEALLBLOCKS(x)

#endif      // _DEBUG

#ifdef __cplusplus
}
#endif      // __cplusplus

#endif      // _MEMDUMPERVALIDATOR_H

