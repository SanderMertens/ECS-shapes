
/*
WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

This file was generated from ShapeType.idl using "rtiddsgen".
The rtiddsgen tool is part of the RTI Connext distribution.
For more information, type 'rtiddsgen -help' at a command shell
or consult the RTI Connext manual.
*/

#include "ShapeTypeSupport.h"
#include "ShapeTypePlugin.h"

/* ========================================================================= */
/**
<<IMPLEMENTATION>>

Defines:   TData,
TDataWriter,
TDataReader,
TTypeSupport

Configure and implement 'ShapeType' support classes.

Note: Only the #defined classes get defined
*/

/* ----------------------------------------------------------------- */
/* DDSDataWriter
*/

/**
<<IMPLEMENTATION >>

Defines:   TDataWriter, TData
*/

/* Requires */
#define TTYPENAME   ShapeTypeTYPENAME

/* Defines */
#define TDataWriter ShapeTypeDataWriter
#define TData       ShapeType

#include "dds_c/generic/dds_c_data_TDataWriter.gen"

#undef TDataWriter
#undef TData

#undef TTYPENAME

/* ----------------------------------------------------------------- */
/* DDSDataReader
*/

/**
<<IMPLEMENTATION >>

Defines:   TDataReader, TDataSeq, TData
*/

/* Requires */
#define TTYPENAME   ShapeTypeTYPENAME

/* Defines */
#define TDataReader ShapeTypeDataReader
#define TDataSeq    ShapeTypeSeq
#define TData       ShapeType

#include "dds_c/generic/dds_c_data_TDataReader.gen"

#undef TDataReader
#undef TDataSeq
#undef TData

#undef TTYPENAME

/* ----------------------------------------------------------------- */
/* TypeSupport

<<IMPLEMENTATION >>

Requires:  TTYPENAME,
TPlugin_new
TPlugin_delete
Defines:   TTypeSupport, TData, TDataReader, TDataWriter
*/

/* Requires */
#define TTYPENAME    ShapeTypeTYPENAME
#define TPlugin_new  ShapeTypePlugin_new
#define TPlugin_delete  ShapeTypePlugin_delete

/* Defines */
#define TTypeSupport ShapeTypeTypeSupport
#define TData        ShapeType
#define TDataReader  ShapeTypeDataReader
#define TDataWriter  ShapeTypeDataWriter
#define TGENERATE_SER_CODE
#define TGENERATE_TYPECODE

#include "dds_c/generic/dds_c_data_TTypeSupport.gen"

#undef TTypeSupport
#undef TData
#undef TDataReader
#undef TDataWriter
#undef TGENERATE_TYPECODE
#undef TGENERATE_SER_CODE
#undef TTYPENAME
#undef TPlugin_new
#undef TPlugin_delete

/* ========================================================================= */
/**
<<IMPLEMENTATION>>

Defines:   TData,
TDataWriter,
TDataReader,
TTypeSupport

Configure and implement 'ShapeTypeExtended' support classes.

Note: Only the #defined classes get defined
*/

/* ----------------------------------------------------------------- */
/* DDSDataWriter
*/

/**
<<IMPLEMENTATION >>

Defines:   TDataWriter, TData
*/

/* Requires */
#define TTYPENAME   ShapeTypeExtendedTYPENAME

/* Defines */
#define TDataWriter ShapeTypeExtendedDataWriter
#define TData       ShapeTypeExtended

#include "dds_c/generic/dds_c_data_TDataWriter.gen"

#undef TDataWriter
#undef TData

#undef TTYPENAME

/* ----------------------------------------------------------------- */
/* DDSDataReader
*/

/**
<<IMPLEMENTATION >>

Defines:   TDataReader, TDataSeq, TData
*/

/* Requires */
#define TTYPENAME   ShapeTypeExtendedTYPENAME

/* Defines */
#define TDataReader ShapeTypeExtendedDataReader
#define TDataSeq    ShapeTypeExtendedSeq
#define TData       ShapeTypeExtended

#include "dds_c/generic/dds_c_data_TDataReader.gen"

#undef TDataReader
#undef TDataSeq
#undef TData

#undef TTYPENAME

/* ----------------------------------------------------------------- */
/* TypeSupport

<<IMPLEMENTATION >>

Requires:  TTYPENAME,
TPlugin_new
TPlugin_delete
Defines:   TTypeSupport, TData, TDataReader, TDataWriter
*/

/* Requires */
#define TTYPENAME    ShapeTypeExtendedTYPENAME
#define TPlugin_new  ShapeTypeExtendedPlugin_new
#define TPlugin_delete  ShapeTypeExtendedPlugin_delete

/* Defines */
#define TTypeSupport ShapeTypeExtendedTypeSupport
#define TData        ShapeTypeExtended
#define TDataReader  ShapeTypeExtendedDataReader
#define TDataWriter  ShapeTypeExtendedDataWriter
#define TGENERATE_SER_CODE
#define TGENERATE_TYPECODE

#include "dds_c/generic/dds_c_data_TTypeSupport.gen"

#undef TTypeSupport
#undef TData
#undef TDataReader
#undef TDataWriter
#undef TGENERATE_TYPECODE
#undef TGENERATE_SER_CODE
#undef TTYPENAME
#undef TPlugin_new
#undef TPlugin_delete

