/************************************************************************
** FILE NAME..... HeapImage.cpp
** 
** (c) COPYRIGHT 
** 
** FUNCTION......... Generate image(jpeg etc.) of heap profile
**
** NOTES............ 
** 
** ASSUMPTIONS...... 
** 
** RESTRICTIONS..... 
**
** LIMITATIONS...... 
** 
** DEVIATIONS....... 
** 
** RETURN VALUES.... 0  - successful
**                   !0 - error
** 
** AUTHOR(S)........ Michael Q Yan
** 
** CHANGES:
** 
************************************************************************/
#ifdef WIN32
//#include <windows.h>
typedef int                   INT32;
typedef unsigned int          UINT32;
#define far

namespace PGSDK
{
	typedef short int INT16;
	typedef void* GraphPtr;
	#define A2D_RANGE_BEGIN		  1

	extern "C" INT16 SetGraphAttr	(	GraphPtr				pGraph,
		INT16						nLayerID,
		INT16						nObjectID,
		INT16						nSeriesID,
		INT16						nGroupID,
		INT16						attrCode,
		void *			pData			);
typedef enum _Attributes2D {
	A2D_AUTOFIT_ALL = A2D_RANGE_BEGIN, 				/* Global control of autofitting. */
	A2D_AUTOFIT_DATATEXT,
	A2D_AUTOFIT_LEGEND,
	A2D_AUTOFIT_O1,
	A2D_AUTOFIT_O2,
	A2D_AUTOFIT_X,
	A2D_AUTOFIT_Y1,
	A2D_AUTOFIT_Y2,
	A2D_AXISSWAP,								/* Interpret series as columns? */
	A2D_BAR_GROUP_SPACING,			/* Intergroup spacing for side by side charts. */
	A2D_BAR_RISER_WIDTH,				/* Bar width (-50 <= x < 0 < x <= 100 ). */
	A2D_COLORBYSERIES,				 	/* Color QDRs by series? */
	A2D_CURVECOLORASSERIES,
	A2D_CURVEMOVING,						/* Moving average line factor. */
	A2D_CURVESMOOTH,						/* Smooth curve factor (0 < x < 16). */
	A2D_DATAFORMAT,							/* Data format specifier. */
	A2D_DATAFORMAT_VERT,				/* Vertical data node format control. */
	A2D_DEPTH_IMS_ANGLE,
	A2D_DEPTH_IMS_COLORMODE,
	A2D_DEPTH_IMS_THICK,
	A2D_DEPTH_MODE,
	A2D_DIRECTION_X,						/* X numeric axis ascending or descending? */
	A2D_DIRECTION_Y1,						/* Y1 numeric axis ascending or descending? */
	A2D_DIRECTION_Y2,						/* Y2 numeric axis ascending or descending? */
	A2D_EXCLUDE_ZERO_X,
	A2D_EXCLUDE_ZERO_Y1,
	A2D_EXCLUDE_ZERO_Y2,
	A2D_FORMATDTXT_X,
	A2D_FORMATDTXT_Y1,
	A2D_FORMATDTXT_Y2,
	A2D_FORMAT_DATATEXT,				/* Set format code for data text. */
	A2D_FORMAT_SDLINE,
	A2D_FORMAT_X,								/* Set format code for X axis scale text. */
	A2D_FORMAT_Y1,							/* Set format code for Y1 axis scale text. */
	A2D_FORMAT_Y2,							/* Set format code for Y2 axis scale text. */
	A2D_FORMAT_Z,								/* Set format code for Z axis scale text. */
	A2D_GRIDLINESORD_O1,				/* O1 ordinal axis gridline control. */
	A2D_GRIDLINESORD_O2,				/* O2 ordinal axis gridline control. */
	A2D_GRIDLINES_X,						/* X numeric axis gridline control. */
	A2D_GRIDLINES_Y1,						/* Y1 numeric axis gridline control. */
	A2D_GRIDLINES_Y2,						/* Y2 numeric axis gridline control. */
	/* TL:11/18/92 added new attributes to control ordinal axis gridlines */
	A2D_GRIDMODEMAJOR_O1,				/* O1 ordinal axis major gridline mode control */
	A2D_GRIDMODEMINOR_O1,				/* O1 ordinal axis minor gridline mode control */
	A2D_HL_COLOR,
	A2D_IGNORE_SERIES,
	A2D_INSET_LGND_ICON,				/* Inset box of legend icon. */
	A2D_INSET_LGND_TEXT,				/* Inset box of legend text. */
	A2D_LABELMODE_O1,
	A2D_LABELMODE_O2,
	A2D_LABELWRAPMODE_O1,				/* TL:11/12/92 multi-line label wrap */
	A2D_LABELWRAPLINES_O1,			/* TL:11/12/92 multi-line label wrap # of lines */
	A2D_LOCATE_LEGEND,					/* Legend surface location. */
	A2D_LOCATE_LINR_TEXT,
	A2D_LOCATE_TITLE_X,					/* X axis title location. */
	A2D_LOCATE_TITLE_Y1,				/* Y1 axis title location. */
	A2D_LOCATE_TITLE_Y2,				/* Y2 axis title location. */
	A2D_LOG_X,									/* X numeric axis logarithmic flag. */
	A2D_LOG_Y1,									/* Y1 numeric axis logarithmic flag. */
	A2D_LOG_Y2,									/* Y2 numeric axis logarithmic flag. */
	A2D_MARKERSHAPE,						/* Marker shape. */
	A2D_MARKERSIZE,							/* Marker size. */
	A2D_ORIENTATION,						/* Horizontal chart flag. */
	A2D_PLACE_O1,
	A2D_PLACE_O2,
	A2D_PLACE_X,
	A2D_PLACE_Y1,
	A2D_PLACE_Y2,
	A2D_PRESDL_CONN,
	A2D_PRESDL_CONNBRK,
	A2D_PRESDL_STEP,
	A2D_PRESDL_STEPBRK,
	A2D_PRESDL_STEPVERT,
	A2D_SCALEBASE,							/* Riser base control. */
	A2D_SCALEEND_X,
	A2D_SCALEEND_Y1,
	A2D_SCALEEND_Y2,
	A2D_SCALEFREQ_BEG_O1,
	A2D_SCALEFREQ_BEG_O2,
	A2D_SCALEFREQ_BEG_X,
	A2D_SCALEFREQ_BEG_Y1,
	A2D_SCALEFREQ_BEG_Y2,
	A2D_SCALEFREQ_END_O1,
	A2D_SCALEFREQ_END_O2,
	A2D_SCALEFREQ_END_X,
	A2D_SCALEFREQ_END_Y1,
	A2D_SCALEFREQ_END_Y2,
	A2D_SCALEFREQ_O1,
	A2D_SCALEFREQ_O2,
	A2D_SCALEFREQ_X,
	A2D_SCALEFREQ_Y1,
	A2D_SCALEFREQ_Y2,
	A2D_SCALE_X,								/* X numeric axis scale control. */
	A2D_SCALE_Y1,								/* Y1 numeric axis scale control. */
	A2D_SCALE_Y2,								/* Y2 numeric axis scale control. */
	A2D_SCIMOVAVG,							/* Generate scientific moving averages. */
	A2D_SDDATALINE_TYPE,
	A2D_SDEMPHASIZED,						/* Series dependant integer - emphasized. */
	A2D_SDLINECONN,							/* Series dependant line BOOLEAN16ean - connected. */
	A2D_SDLINECURV,							/* Series dependant line BOOLEAN16ean - smooth curve. */
	A2D_SDLINELINR,
	A2D_SDLINELINR_CORR,
	A2D_SDLINELINR_EXP,
	A2D_SDLINELINR_FORMULA,
	A2D_SDLINELINR_LINE,				/* Series dependant line BOOLEAN16ean - linear regression. */
	A2D_SDLINELINR_LOG,
	A2D_SDLINELINR_NATLOG,
	A2D_SDLINELINR_NPOLY,
	A2D_SDLINELINR_NPOLYFAC,
	A2D_SDLINEMEAN,							/* Series dependant line BOOLEAN16ean - mean. */
	A2D_SDLINEMOVA,							/* Series dependant line BOOLEAN16ean - moving average. */
	A2D_SDLINESTDD,							/* Series dependant line BOOLEAN16ean - standard deviation. */
	A2D_SHADOW_OFFSETS,					/* Riser shadow offsets. */
	A2D_SHOW_DATATEXT,				 	/* Data text show flag. */
	A2D_SHOW_DIVBIPOLAR,				/* Bipolar division line flag. */
	A2D_SHOW_ERRORBAR,
	A2D_SHOW_LEGEND,						/* Legend box show flag. */
	A2D_SHOW_O1,								/* Ordinal axis 1 show flag. */
	A2D_SHOW_O2,								/* Ordinal axis 2 show flag. */
	A2D_SHOW_OFFSCALE_X,
	A2D_SHOW_OFFSCALE_Y1,
	A2D_SHOW_OFFSCALE_Y2,
	A2D_SHOW_QUADRANTS,					/* DW: 4/22/94 */
	A2D_SHOW_SHADOWS,						/* Riser shadow show flag. */
	A2D_SHOW_TITLE_X,						/* X axis title text show flag. */
	A2D_SHOW_TITLE_Y1,					/* Y1 axis title text show flag. */
	A2D_SHOW_TITLE_Y2,					/* Y2 axis title text show flag. */
	A2D_SHOW_X,									/* Numeric X axis show flag. */
	A2D_SHOW_Y1,								/* Numeric primary Y axis show flag. */
	A2D_SHOW_Y2,								/* Numeric secondary Y axis show flag. */
	A2D_SHOW_ZL_X,
	A2D_SHOW_ZL_Y1,
	A2D_SHOW_ZL_Y2,
	A2D_SIDE_O1,								/* O1 axis label placement. */
	A2D_SIDE_O2,								/* O2 axis label placement. */
	A2D_SIDE_X,									/* X axis label placement. */
	A2D_SIDE_Y1,								/* Y1 axis label placement. */
	A2D_SIDE_Y2,								/* Y1 axis label placement. */
	A2D_SIZE_ERRORBARS,
	A2D_SPLITY,									/* Y Axis association for Dual Y charts. */
	A2D_SPLITY_ND,
	A2D_SQUARE_LGND_ICONS,
	A2D_STAGGER_O1,							/* Staggered axis text flag. */
	A2D_STAGGER_O2,							/* Staggered axis text flag. */
	A2D_STAGGER_X, 							/* Staggered axis text flag. */
	A2D_STAGGER_Y1,							/* Staggered axis text flag. */
	A2D_STAGGER_Y2,							/* Staggered axis text flag. */
	A2D_SYMBOL_FORSERIES,
	A2D_UNIFORM_QDRBORDERS,
	A2D_UNIFORM_QDRSHAPES,

	ABA_PICTOGRAPH,

	ABP_CATTABLE_LAYOUT,
	ABP_COMPRESSION,
	ABP_DATAMARKER_SPACING,
	ABP_GROUPTABLE_FAR,
	ABP_GROUPTABLE_NEAR,
	ABP_GROUP_SPLITY_ND,
	ABP_JITTER,
	ABP_SHOW_DATAMARKERS,
	ABP_SHOW_GROUPTABLE,
	ABP_SHOW_HINGEBOXES,
	ABP_SHOW_HINGES,
	ABP_SHOW_HINGETAILS,
	ABP_SHOW_MEDIAN,
	ABP_SHOW_OUTLIERS,

	ACT_BCLIPSURFACE,  /* Clip surface */
	ACT_BSHOWLABEL,			 /* on/off Contour Label */
	ACT_CLEVEL,        /* Contour level CLevelLookClass */
	ACT_CPOINT,        /* Contour point */
	ACT_CSURFACE,        /* Contour surface */
	ACT_FLMAJORINTAUTO,
	ACT_FLMAJORINTMANU,
	ACT_FLMINORINTAUTO,
	ACT_FLMINORINTMANU,
	ACT_FLMAXAUTO,		/* Auto contour level max */
	ACT_FLMAXMANU,		/* Mnual contour level max */
	ACT_FLMINAUTO,		/* Auto contour level min */
	ACT_FLMINMANU,		/* Mnual contour level min */
	ACT_NCONTOURS,     /* Number of Contour levels 0 < x */
	ACT_NCPOINTS,      /* Number of Contour points 0 <= x */
	ACT_NINDEX,         /* set index for AttrSetOperation */
	ACT_NLABELSEG,		 /* Number of contour labels every nTh segment > 0 */
	ACT_NLFORMAT,
	ACT_NOPERATION,		 /* Set operation format */
	ACT_NSMOOTH,			 /* Contour smooth factor (1 <= x <= 10). */
	ACT_NSURFACES,     /* Number of SURFACES 0 < x */
	ACT_NSURFACESFLAG,
	ACT_XSTART,        /* Contour x-axis starting point */
	ACT_XSTEP,         /* Contour x-axis unit interval */
	ACT_YSTART,        /* Contour y-axis starting point > 0.0 */
	ACT_YSTEP,         /* Contour y-axis unit interval > 0.0 */

	API_AUTOFIT_LBLPIE,
	API_DEPTH,
	API_FEELER_CENTER,
	API_FEELER_HORZ,
	API_FEELER_RADIAL,
	API_FORMAT_DATATEXT,
	API_FORMAT_RINGTEXT,
	API_HOLESIZE,
	API_INSET_LBLPIE,
	API_INSET_PIE,
	API_PIESPERROW,
	API_ROTATE,
	API_SHOW_AS_COLUMN,	/* 5mar98DW: For Corel Pie Column combo charts. */
	API_SHOW_FEELER,
	API_SHOW_LBLFEELER,
	API_SHOW_LBLPIE,
	API_SHOW_LBLRING,
	API_SLICE_DELETE,
	API_SLICE_MOVE,
	API_SLICE_RESTORE,
	API_TILT,
	API_TILT_ON,
	API_DRAW_CLOCKWISE,

	APL_ANGLE,				/* Angle in Degree or Radian */
	APL_AXIS,				/* Polar coordinate Axis wanted or not */
	APL_AXIS_CIRCLES,		/* Number of Polar Axis Circles */
	APL_AXIS_THICKS,		/* Number of Polar Axis Thicks */
	APL_LINES,				/* Connecting polar lines graphed or not */
	APL_LINE_FORMAT,		/* Format to connect for connecting lines. 0 = all, 1 = by series, 2 = by groups */
	APL_SMOOTH,				/* Smooth factor for connecting lines */

	ASM_HLWIDTH,
	ASM_ITEM_DTXT,
	ASM_ITEM_SDLINE,
	ASM_OCHEIGHT,
	ASM_OCWIDTH,
	ASM_SHOW_CLOSE,
	ASM_SHOW_OPEN,

	ASP_INSET_MAP,

	ATC_AUTOFIT_ALL,
	ATC_AUTOFIT_CELL,
	ATC_AUTOFIT_COLHEAD,
	ATC_AUTOFIT_ROWHEAD,
	ATC_AUTOFIT_SUBJECT,
	ATC_DIVISIONS,
	ATC_INSET_TABLE,
	ATC_LINECONTROL,
	ATC_UNIFORMCOLS,
	ATC_UNIFORMROWS,
	A2D_IGNORE_GROUPS,
	ATC_AUTOFIT_TEXT,
	A2D_BUBBLEGRID_COUNT_X,			/* bubble quadrants */
	A2D_BUBBLEGRID_COUNT_Y,			/* bubble quadrants */
	A2D_BUBBLEGRID_POSITION_X,	/* bubble quadrant line position when count is 2 */
	A2D_BUBBLEGRID_POSITION_Y,	/* bubble quadrant line position when count is 2 */
	A2D_SHOW_BUBBLEGRID,				/* TL:02/19/95 */

	AMD_OPTIONS,								/* 20jan96DW: Multi-dimensional label options */
	A2D_SD_SHOWDATATEXT, 				/* 13nov97DW: Series-Dependent DataText */
	A2D_SHOW_TITLE_SERIES, 			/* Legend Title On/Off flag. */
	A2D_CONNECT_STACKBARS,			/*1apr98DW: For Corel */
	A2D_LEGEND_FANCYBOX,

	/*  08sep98/jst: NO MORE ATTRIBUTES WILL FIT HERE ... see (cro_defs.h) ... 9 bits ??? */

	MAX_2D_ATTRIBUTE
} Attributes2D;
}
//#include "pg32.h"
#endif

#include "ChartCtrl.h"
#include "MallocHeap.h"

#ifndef WIN32
#include "pg32.h"
#endif

#ifdef WIN32
__declspec(dllexport) void* ChartCtrl::GetGraphPtr() { return mGraphPtr; }
#endif

static const char* gPageTypeNames[] =
{
	"V",
	"F",
	"M",
	"E",
	"S",
	"S",
	"F",
	"X"
};

#define NUM_KINDS_BLOCK 3
static const char* gHeapRowLabels[NUM_KINDS_BLOCK] =
{
	"InUse",
	"Free",
	"Reserve"
};

#define NUM_COLS 64

static const unsigned int gMaxValue = 65536;
static const unsigned int gGridStep = 4096;

static void allege(bool ibCondition)
{
	if(ibCondition == false)
	{
		::fprintf(stderr, "Allege failed. Aborting ...\n");
		::abort();
	}
}

static void set_attr_graph(ChartCtrl* ipChart)
{
	short lbTrue = true;
	allege(ipChart->SetGraphAttr(A_SHOW_TITLE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A_SHOW_SUBTITLE, &lbTrue)==1);
	//allege(ipChart->SetGraphAttr(A_SHOW_FOOTNOTE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_LEGEND, &lbTrue)==1);
	// Don't show the data value in the graph
	//allege(ipChart->SetGraphAttr(A2D_SHOW_DATATEXT, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_TITLE_X, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_TITLE_Y1, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_TITLE_Y2, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_COLTITLE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_ROWTITLE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_LEFTTITLE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_RIGHTTITLE, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_ROWHEADERS, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_COLHEADERS, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_LEFTNUMBERS, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A3D_SHOWTEXT_RIGHTNUMBERS, &lbTrue)==1);

	allege(ipChart->SetGraphAttr(A2D_SHOW_O1, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_X, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_Y1, &lbTrue)==1);
	allege(ipChart->SetGraphAttr(A2D_SHOW_Y2, &lbTrue)==1);

	allege(ipChart->SetGraphAttr(API_SHOW_LBLFEELER, &lbTrue)==1);

	const char* arial = "arial.ttf";
	allege(ipChart->SetGraphAttr(GRAPH_LAYER, O5D_LBLTITLE, NULL_SERIESID, NULL_GROUPID, A_FONTNAME, const_cast<char*>(arial))==1);

	lbTrue = FONTSTYLE_BOLD | FONTSTYLE_ITALIC;
	allege(ipChart->SetGraphAttr(GRAPH_LAYER, O5D_LBLSUBTITLE, NULL_SERIESID, NULL_GROUPID, A_FONTSTYLE, &lbTrue)==1);

	//lbTrue = FONTALIGN_LEFT;
	//allege(ipChart->SetGraphAttr(GRAPH_LAYER, O5D_LBLFOOTNOTE, NULL_SERIESID, NULL_GROUPID, A_FONTALIGN, &lbTrue)==1);

	lbTrue = 16;
	allege(ipChart->SetGraphAttr(GRAPH_LAYER, O5D_LBLTITLE, NULL_SERIESID, NULL_GROUPID, A_FONTSIZE_POINT_100, &lbTrue)==1);

	//RGBColor rgbVal = { 0xFF, 0, 0xFF };
	//allege(ipChart->SetGraphAttr(GRAPH_LAYER, O5D_LBLFOOTNOTE, NULL_SERIESID, NULL_GROUPID, A_FONTCOLOR_RGB, &rgbVal)==1);

	GridStepValuesInfo gtv = { 1, (double)gGridStep };
	allege(ipChart->SetGraphAttr(A2D_GRID_STEP_Y1, &gtv)==1);

	//ScaleValuesInfo sv = { 0, (double)gMaxValue, (double)gGridStep };
	ScaleValuesInfo sv = { 1, (double)gMaxValue, 0 };
	allege(ipChart->SetGraphAttr(A2D_SCALE_Y1, &sv)==1);
}


void InitHeapImage()
{
	ChartInitialize();
}

void FiniHeapIMage()
{
	ChartFinalize();
}

bool GenHeapBar(const char* ipImageBaseName, HeapInfo* ipHeapInfo, PAGESET* ipPageSet)
{
	// The base name is supplied by caller. Add another sequence number because there
	// maybe more data one image can display, so we have to split into multiple image files
	char lImageName[64];
	sprintf(lImageName, "%s.00.jpg", ipImageBaseName);

	int lTotalPages = ipPageSet->size();
	if(lTotalPages < 1)
	{
		return false;
	}

	ChartCtrlPtr lChart(new ChartCtrl());

	int lnRows = NUM_KINDS_BLOCK, lnCols = NUM_COLS;

	int ret = lChart->SetGraphDataInfo(lnRows, lnCols);
	allege(ret==0);

	// graph template file
	const char* lpPath = "Default.3tf";
	ret = lChart->Load_TIFFGraph(REQUEST_TDC, const_cast<char*>(lpPath));
	allege(ret==0);

	ret = lChart->SetGraphDataInfo(lnRows, lnCols);
	allege(ret==0);

	// choose one of the bar graphs
	int lnType = Horizontal_Bar_Stacked; // 22
	ret = lChart->SetGraphAttr(A_GRAPH_PRESET, &lnType);
	allege(ret==1);

	set_attr_graph(lChart.Get());

	// Adjust spacing
	short BarSpace = 0; //25% space between groups
	short BarWidth = 0; //75% space between risers within each group
	ret = PGSDK::SetGraphAttr(lChart->GetGraphPtr(), GRAPH_LAYER,
	     	NULL_OBJECTID, NULL_SERIESID, NULL_GROUPID,
	     	PGSDK::A2D_BAR_RISER_WIDTH, &BarWidth);
	allege(ret==1);

	ret = PGSDK::SetGraphAttr(lChart->GetGraphPtr(), GRAPH_LAYER,
			NULL_OBJECTID, NULL_SERIESID, NULL_GROUPID,
			PGSDK::A2D_BAR_GROUP_SPACING, &BarSpace);
	allege(ret==1);

	// Titles/lables
	char buffer[64];
	allege(lChart->SetGraphTitle(const_cast<char*>("Heap Profile"))==1);
	sprintf(buffer, "Context(%d,%d) Request(%ld) HeapTop(0x%lx)",
					ipHeapInfo->mPool, ipHeapInfo->mSubPool,
					ipHeapInfo->mUserRequestSize, ipHeapInfo->mHeapTop);
	allege(lChart->SetGraphSubTitle(buffer)==1);

	// Set Legend
	for (int i = 0; i < lnRows; i++)
	{
		allege(lChart->SetGraphSeriesLabel(i, const_cast<char*>(gHeapRowLabels[i]))==1);
	}

	// Set page address, type, values
	PAGESET::iterator it = ipPageSet->begin();
	for(int j=0; j<lnCols; j++, it++)
	{
		if(j<lTotalPages && it!=ipPageSet->end())
		{
			PageInfo* lpPageInfo = *it;
			// Set Legend
			sprintf(buffer, "0x%lx (%d,%d) %s(%d)",
						lpPageInfo->mPageStart,
						lpPageInfo->mPool, lpPageInfo->mSubPool,
						gPageTypeNames[lpPageInfo->mPageType], lpPageInfo->mMaxFreeBlockSize);
			allege(lChart->SetGraphGroupsLabel(j, buffer)==1);
			//allege(lChart->SetGraphGroupsLabel(j, NULL)==1);
			// Set values
			unsigned int lLogicPageSize = gMaxValue;
			unsigned int lReservedSize = lpPageInfo->mPageEnd - lpPageInfo->mPageStart - lpPageInfo->mInUseBytes - lpPageInfo->mFreeBytes;
			// We have to spread out a big page into several bars
			if(lpPageInfo->mPageEnd - lpPageInfo->mPageStart > lLogicPageSize)
			{
				unsigned int lInUse = lpPageInfo->mInUseBytes;
				unsigned int lLeftOver = gMaxValue;
				while(lInUse>0 && j<lnCols)
				{
					if(lInUse >= lLogicPageSize)
					{
						allege(lChart->SetGraphRowColData(0, j, lLogicPageSize)==1);
						allege(lChart->SetGraphRowColData(1, j, 0)==1);
						allege(lChart->SetGraphRowColData(2, j, 0)==1);
						lInUse -= lLogicPageSize;
						j++;
						allege(lChart->SetGraphGroupsLabel(j, NULL)==1);
						continue;
					}
					else
					{
						allege(lChart->SetGraphRowColData(0, j, lInUse)==1);
						allege(lChart->SetGraphRowColData(1, j, lpPageInfo->mFreeBytes)==1);
						allege(lChart->SetGraphRowColData(2, j, lReservedSize)==1);
						lInUse = 0;
						break;
					}
				}
			}
			// Normal page is easier.
			else
			{
				allege(lChart->SetGraphRowColData(0, j, lpPageInfo->mInUseBytes)==1);
				allege(lChart->SetGraphRowColData(1, j, lpPageInfo->mFreeBytes)==1);
				allege(lChart->SetGraphRowColData(2, j, lReservedSize)==1);
			}
		}
		else
		{
			// Don't have so many pages to display
			allege(lChart->SetGraphGroupsLabel(j, NULL)==1);
			allege(lChart->SetGraphRowColData(0, j, 0)==1);
			allege(lChart->SetGraphRowColData(1, j, 0)==1);
			allege(lChart->SetGraphRowColData(2, j, 0)==1);
		}
	}


	int lnWidth = 1024, lnHeight = 800;
	int err = lChart->PlaceDefaultElements(lnWidth, lnHeight);
	allege(err==0);

	lChart->SaveImage(lnWidth, lnHeight, EXPORT_FILE_JPEG, const_cast<char*>(lImageName));

	return true;
}
