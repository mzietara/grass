/* save.c                                                               */

#undef TRACE
#undef DEBUG

#include <string.h>
#include <grass/gis.h>
#include <grass/glocale.h>
#include "ransurf.h"


void SaveMap(int NumMap, int MapSeed)
{
    int Index, Row, Col, NormIndex;
    int LowColor, HighColor;
    double DownInterval, UpInterval, Value = 0, Ratio, MeanMod;
    struct Categories Cats;
    struct Colors Colr;
    char String[80], Label[240];
    struct History history;

    FUNCTION(SaveMap);

    OutFD = G_open_cell_new(OutNames[NumMap]);
    if (OutFD < 0)
	G_fatal_error("%s: unable to open new raster map [%s]",
		      G_program_name(), OutNames[NumMap]);

    MeanMod = 0.0;
    INT(FDM);
    if (FDM == -1) {
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = Surface[Row][Col];
		MeanMod += Value;
	    }
	}

	MeanMod /= MapCount;
	/* Value = (Value - MeanMod) / FilterSD + MeanMod / FilterSD; */
	Value /= FilterSD;
	DownInterval = UpInterval = Value;
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = Surface[Row][Col];
		/*
		   Value = (Value - MeanMod) / FilterSD
		   + MeanMod / FilterSD;
		 */
		Value /= FilterSD;
		Surface[Row][Col] = Value;

		if (UpInterval < Value)
		    UpInterval = Value;

		if (DownInterval > Value)
		    DownInterval = Value;
	    }
	}
    }
    else {
	for (Row = 0; Row < Rs; Row++) {
	    G_get_map_row_nomask(FDM, CellBuffer, Row);
	    for (Col = 0; Col < Cs; Col++) {
		if (CellBuffer[Col] != 0) {
		    Value = Surface[Row][Col];
		    MeanMod += Value;
		}
	    }
	}

	MeanMod /= MapCount;
	DOUBLE(MeanMod);
	DOUBLE(FilterSD);
	/* Value = (Value - MeanMod) / FilterSD + MeanMod / FilterSD; */
	Value /= FilterSD;
	DOUBLE(Value);
	RETURN;
	DownInterval = UpInterval = Value;

	for (Row = 0; Row < Rs; Row++) {
	    G_get_map_row_nomask(FDM, CellBuffer, Row);
	    for (Col = 0; Col < Cs; Col++) {
		if (CellBuffer[Col] != 0) {
		    Value = Surface[Row][Col];
		    /*
		       Value = (Value - MeanMod) / FilterSD
		       + MeanMod / FilterSD;
		     */
		    Value /= FilterSD;
		    Surface[Row][Col] = Value;

		    if (UpInterval < Value)
			UpInterval = Value;

		    if (DownInterval > Value)
			DownInterval = Value;
		}
	    }
	}
    }

    G_message(_("%s: saving [%s] raster map layer.\nPercent complete:"),
	      G_program_name(), OutNames[NumMap]);

    for (Index = 0; Index < CatInfo.NumCat; Index++) {
	CatInfo.Max[Index] = DownInterval;
	CatInfo.Min[Index] = UpInterval;
	CatInfo.NumValue[Index] = 0;
	CatInfo.Average[Index] = 0.0;
    }

    if (DownInterval == UpInterval)
	UpInterval += .1;

    if (!Uniform->answer) {
	FUNCTION(NOT_UNIFORM);
	/* normal distribution */
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = Surface[Row][Col];
		if (Value > UpInterval) {
		    Value = UpInterval;
		}
		else if (Value < DownInterval) {
		    Value = DownInterval;
		}

		Ratio = (Value - DownInterval) / (UpInterval - DownInterval);

		/* Ratio in the range of [0..1] */
		Index = (int)((Ratio * CatInfo.NumCat) - .5);
		CatInfo.NumValue[Index]++;
		CatInfo.Average[Index] += Value;

		if (Value > CatInfo.Max[Index])
		    CatInfo.Max[Index] = Value;

		if (Value < CatInfo.Min[Index])
		    CatInfo.Min[Index] = Value;

		Surface[Row][Col] = 1 + Index;
	    }
	}
    }
    else {
	/* mapping to cumulative normal distribution function */
	for (Row = 0; Row < Rs; Row++) {
	    for (Col = 0; Col < Cs; Col++) {
		Value = Surface[Row][Col];
		Ratio = (double)(Value - MIN_INTERVAL) /
		    (MAX_INTERVAL - MIN_INTERVAL);
		Index = (int)(Ratio * (SIZE_OF_DISTRIBUTION - 1));
		/* Norm[Index] is always smaller than 1.  */
		NormIndex = (int)(Norm[Index] * CatInfo.NumCat);
		/* record the catogory information */
		CatInfo.NumValue[NormIndex]++;
		CatInfo.Average[NormIndex] += Value;

		if (Value > CatInfo.Max[NormIndex])
		    CatInfo.Max[NormIndex] = Value;

		if (Value < CatInfo.Min[NormIndex])
		    CatInfo.Min[NormIndex] = Value;

		/* NormIndex in range of [0 .. (CatInfo.NumCat-1)] */
		Surface[Row][Col] = 1 + NormIndex;
	    }
	}
    }

    for (Row = 0; Row < Rs; Row++) {
	for (Col = 0; Col < Cs; Col++) {
	    CellBuffer[Col] = (CELL) Surface[Row][Col];
	}
	G_put_raster_row(OutFD, CellBuffer, CELL_TYPE);
	G_percent(Row + 1, Rs, 1);
    }

    G_close_cell(OutFD);
    G_short_history(OutNames[NumMap], "raster", &history);
    G_command_history(&history);
    G_write_history(OutNames[NumMap], &history);

    strcpy(Label, Buf);
    sprintf(String, " seed=%d", MapSeed);
    strcat(Label, String);

    /*
       if( NumMap == 0 && Theory > 0)
       TheoryCovariance( TheoryName, Label);
     */

    G_init_cats(CatInfo.NumCat, Label, &Cats);
    for (Index = 0; Index < CatInfo.NumCat; Index++) {
	if (CatInfo.NumValue[Index] != 0) {
	    CatInfo.Average[Index] /= CatInfo.NumValue[Index];
	    sprintf(Label, "%+lf %+lf to %+lf",
		    CatInfo.Average[Index],
		    CatInfo.Min[Index], CatInfo.Max[Index]);
	    G_set_cat(1 + Index, Label, &Cats);
	}
    }

    G_write_cats(OutNames[NumMap], &Cats);
    G_init_colors(&Colr);
    LowColor = (int)(127.5 * (CatInfo.Average[0] + 3.5) / 3.5);
    HighColor = (int)(255.0 - 127.5 *
		      (3.5 - CatInfo.Average[CatInfo.NumCat - 1]) / 3.5);

    if (Uniform->answer || LowColor < 0)
	LowColor = 0;
    if (Uniform->answer || HighColor > 255)
	HighColor = 255;
    INT(LowColor);
    INT(HighColor);

    G_add_color_rule(1, LowColor, LowColor, LowColor,
		     High, HighColor, HighColor, HighColor, &Colr);

    if (G_write_colors(OutNames[NumMap], G_mapset(), &Colr) == -1)
	G_warning("%s: unable to write colr file for %s\n",
		  G_program_name(), OutNames[NumMap]);
}
