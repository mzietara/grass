/*!
  \file lib/display/r_raster.c

  \brief Display Library - Raster graphics subroutines

  (C) 2001-2009, 2011 by the GRASS Development Team

  This program is free software under the GNU General Public License
  (>=v2). Read the file COPYING that comes with GRASS for details.

  \author Original author CERL
*/

#include <grass/config.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <grass/gis.h>
#include <grass/glocale.h>
#include <grass/display.h>

#include "driver.h"

extern const struct driver *PNG_Driver(void);
extern const struct driver *PS_Driver(void);
extern const struct driver *HTML_Driver(void);
#ifdef USE_CAIRO
extern const struct driver *Cairo_Driver(void);
#endif

static int read_env_file(const char *);

static void init(void)
{
    const char *fenc = getenv("GRASS_ENCODING");
    const char *font = getenv("GRASS_FONT");
    const char *line_width = getenv("GRASS_LINE_WIDTH");
    const char *text_size = getenv("GRASS_TEXT_SIZE");
    const char *frame = getenv("GRASS_FRAME");

    D_font(font ? font : "romans");

    if (fenc)
	D_encoding(fenc);

    if (line_width)
	COM_Line_width(atof(line_width));

    if (text_size) {
	double s = atof(text_size);
	D_text_size(s, s);
    }

    D_text_rotation(0);

    if (frame) {
	double t, b, l, r;
	sscanf(frame, "%lf,%lf,%lf,%lf", &t, &b, &l, &r);
	COM_Set_window(t, b, l, r);
    }
}

int read_env_file(const char *path)
{
    FILE *fd;
    char buf[1024];
    char **token;
    
    fd = fopen(path, "r");
    if (!fd)
	return -1;

    token = NULL;
    while (G_getl2(buf, sizeof(buf) - 1, fd) != 0) {
	token = G_tokenize(buf, "=");
	if (G_number_of_tokens(token) != 2)
	    continue;
	
	setenv(token[0], token[1], 1);
	G_free_tokens(token);
	token = NULL;
    }

    return 0;
}

/*!
  \brief Open display driver

  Default display driver is Cairo, if not available PNG is used.

  \todo Replace strncmp by G_strncasecmp()
  
  \return 0 on success
  \return 1 no monitor defined
*/
int D_open_driver(void)
{
    const char *p, *m;
    
    p = getenv("GRASS_RENDER_IMMEDIATE");
    m = G__getenv("MONITOR");
    if (m) {
	char *env;
	const char *v;
	
	if (p)
	    G_warning(_("%s variable defined, %s ignored"),
		      "MONITOR", "GRASS_RENDER_IMMEDIATE");
	env = NULL;
	G_asprintf(&env, "MONITOR_%s_MAPFILE", m);
	v = G__getenv(env);
	if (strncmp(m, "wx", 2) == 0) 
	    p = NULL; /* use default display driver */
	else
	    p = m;
	
	if (v) {
	    if (p && G_strcasecmp(p, "ps") == 0)
		setenv("GRASS_PSFILE", v, 1);
	    else
		setenv("GRASS_PNGFILE", v, 1);
	}
	
	G_asprintf(&env, "MONITOR_%s_ENVFILE", m);
	v = G__getenv(env);
	if (v) 
	    read_env_file(v);
    }
    
    const struct driver *drv =
	(p && G_strcasecmp(p, "PNG")   == 0) ? PNG_Driver() :
	(p && G_strcasecmp(p, "PS")    == 0) ? PS_Driver() :
	(p && G_strcasecmp(p, "HTML")  == 0) ? HTML_Driver() :
#ifdef USE_CAIRO
	(p && G_strcasecmp(p, "cairo") == 0) ? Cairo_Driver() :
	Cairo_Driver();
#else
	PNG_Driver();
#endif
	
    if (p && G_strcasecmp(drv->name, p) != 0)
	G_warning(_("Unknown display driver <%s>"), p);
    G_verbose_message(_("Using display driver <%s>..."), drv->name);
    LIB_init(drv);

    init();

    if (!m)
	return 1;

    return 0;
}

/*!
  \brief Close display driver

  If GRASS_NOTIFY is defined, run notifier.
*/
void D_close_driver(void)
{
    const char *cmd = getenv("GRASS_NOTIFY");

    COM_Graph_close();

    if (cmd)
	system(cmd);
}

int D_save_command(const char *cmd)
{
    const char *mon_name, *mon_cmd;
    char *env;
    FILE *fd;

    G_debug(1, "D_save_command(): %s", cmd);

    mon_name = G__getenv("MONITOR");
    if (!mon_name)
	return 0;
    
    env = NULL;
    G_asprintf(&env, "MONITOR_%s_CMDFILE", mon_name);
    mon_cmd = G__getenv(env);
    if (!mon_cmd)
	return 0;
    
    fd = fopen(mon_cmd, "a");
    if (!fd) {
	G_warning(_("Unable to open file '%s'"), mon_cmd);
	return -1;
    }
    fprintf(fd, "%s\n", cmd);
    fclose(fd);

    return 1;
}

/*!
  \brief Erase display (internal use only)
*/
void D__erase(void)
{
    COM_Erase();
}

/*!
  \brief Set text size (width and height)
 
  \param width text pixel width
  \param height text pixel height
*/
void D_text_size(double width, double height)
{
    COM_Text_size(width, height);
}

/*!
  \brief Set text rotation

  \param rotation value
*/
void D_text_rotation(double rotation)
{
    COM_Text_rotation(rotation);
}

/*!
  \brief Get clipping frame
  
  \param[out] t top
  \param[out] b bottom
  \param[out] l left
  \param[out] r right
*/
void D_get_window(double *t, double *b, double *l, double *r)
{
    return COM_Get_window(t, b, l, r);
}

/*!
  \brief Draw text
  
  Writes <em>text</em> in the current color and font, at the current text
  width and height, starting at the current screen location.
  
  \param text text to be drawn
*/
void D_text(const char *text)
{
    COM_Text(text);
}

/*!
  \brief Choose font
 
  Set current font to <em>font name</em>.
  
  \param name font name
*/
void D_font(const char *name)
{
    COM_Set_font(name);
}

/*!
  \brief Set encoding

  \param name encoding name
*/
void D_encoding(const char *name)
{
    COM_Set_encoding(name);
}

/*!
  \brief Get font list

  \param[out] list list of font names
  \param[out] number of items in the list
*/
void D_font_list(char ***list, int *count)
{
    COM_Font_list(list, count);
}

/*!
  \brief Get font info

  \param[out] list list of font info
  \param[out] number of items in the list
*/
void D_font_info(char ***list, int *count)
{
    COM_Font_info(list, count);
}
