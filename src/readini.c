/* vim:expandtab:ts=2 sw=2:
*/
/*  Grafx2 - The Ultimate 256-color bitmap paint program

	Copyright owned by various GrafX2 authors, see COPYRIGHT.txt for details.

    Grafx2 is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; version 2
    of the License.

    Grafx2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grafx2; if not, see <http://www.gnu.org/licenses/>
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(_MSC_VER)
#define strdup _strdup
#endif

#include "const.h"
#include "errors.h"
#include "global.h"
#include "misc.h"
#include "readini.h"
#include "setup.h"
#include "realpath.h"
#include "io.h"
#include "windows.h"
#include "gfx2log.h"
#include "gfx2mem.h"


/**
 * Clean a line of the ini file.
 *
 * - Suppress all leading and trailing spaces
 * - convert the key to uppercase
 *
 * Comments start with ";". The result will be in the following format :
 * <tt>KEY=value</tt>
 *
 * @param[in,out] str
 * @param[in] keep_comments if set, the comments are kept
 */
void Load_INI_clear_string(char * str, byte keep_comments)
{
  int index;
  int equal_found=0;

  for (index=0;str[index]!='\0';)
  {
    if (str[index]=='=')
    {
      equal_found=1;
      index++;
      // On enleve les espaces après le '='
      while (str[index]==' ' || str[index]=='\t')
        memmove(str+index,str+index+1,strlen(str+index));
    }
    else if ((str[index]==' ' && !equal_found) || (str[index]=='\t'))
    {
      // Suppression d'un espace ou d'un tab:
      memmove(str+index,str+index+1,strlen(str+index));
    }
    else if (!keep_comments && ((str[index]==';') || (str[index]=='#')))
    {
      // Comment
      str[index]='\0';
    }
    else if ((str[index]=='\r') || (str[index]=='\n'))
    {
      // Line break
      str[index]='\0';
    }
    else
    {
      if (!equal_found)
      {
        // Passage en majuscule d'un caractère:
#ifndef GCWZERO  //this causes gcw to crash
        str[index]=toupper((int)str[index]);
#endif
      }
      index++;
    }
  }
  // On enlève les espaces avant la fin de chaine
  while (index>0 && (str[index-1]==' ' || str[index-1]=='\t'))
  {
    index--;
    str[index]='\0';
  }
}

/**
 * Search for a substring in a string
 *
 * @param buffer the string to search into
 * @param pattern the string to search for
 * @return 0 if the pattern was not found
 * @return >0 the position (+1) where the pattern was found
 */
int Load_INI_seek_pattern(const char * buffer,const char * pattern)
{
  int buffer_index;
  int pattern_index;

  // A partir de chaque lettre de la chaîne buffer
  for (buffer_index=0;buffer[buffer_index]!='\0';buffer_index++)
  {
    // On regarde si la chaîne pattern est équivalente à la position courante
    // de la chaîne buffer:
    for (pattern_index=0;(pattern[pattern_index]!='\0') && (buffer[buffer_index+pattern_index]==pattern[pattern_index]);pattern_index++);

    // Si on a trouvé la chaîne pattern dans la chaîne buffer, on renvoie la
    // position à laquelle on l'a trouvée (+1 pour que si on la trouve au
    // début ça ne renvoie pas la même chose que si on ne l'avait pas
    // trouvée):
    if (pattern[pattern_index]=='\0')
      return (buffer_index+1);
  }

  // Si on ne l'a pas trouvée, on renvoie 0:
  return 0;
}

/**
 * Read lines until a group is found
 *
 * @return 0 when the group is reached
 * @return @ref ERROR_INI_CORRUPTED if the group is not found
 */
static int Load_INI_reach_group(FILE * file,char * buffer,const char * group)
{
  int    stop_seek;
  char * group_upper;
  char * upper_buffer;

  // On alloue les zones de mémoire:
  group_upper=(char *)malloc(1024);
  upper_buffer=(char *)malloc(1024);

  // On commence par se faire une version majuscule du groupe à rechercher:
  strcpy(group_upper,group);
  Load_INI_clear_string(group_upper, 0);

  stop_seek=0;
  do
  {
    // On lit une ligne dans le fichier:
    if (fgets(buffer,1024,file)==0)
    {
      free(upper_buffer);
      free(group_upper);
      return ERROR_INI_CORRUPTED;
    }

    Line_number_in_INI_file++;

    // On s'en fait une version en majuscule:
    strcpy(upper_buffer,buffer);
    Load_INI_clear_string(upper_buffer, 0);

    // On compare la chaîne avec le groupe recherché:
    stop_seek=Load_INI_seek_pattern(upper_buffer,group_upper);
  }
  while (!stop_seek);

  free(upper_buffer);
  free(group_upper);

  return 0;
}

///
/// Find the next string in the .INI file.
/// @param file INI file currently opened
/// @param buffer Current text buffer, preserved from one call to the next
/// @param option_name string to search
/// @param return_code the found value will be copied there. (must be allocaed)
/// @param raw_text Boolean: true to return the raw value (up to end-of-line), false to strip comments.
/// @return 0 when OK
/// @return @ref ERROR_INI_CORRUPTED if the option is not found
static int Load_INI_get_string(FILE * file,char * buffer,const char * option_name,char * return_code, byte raw_text)
{
  int    stop_seek;
  char * option_upper;
  char * upper_buffer;
  int    buffer_index;

  // On alloue les zones de mémoire:
  option_upper=(char *)malloc(1024);
  upper_buffer=(char *)malloc(1024);

  // On commence par se faire une version majuscule de l'option à rechercher:
  strcpy(option_upper,option_name);
  Load_INI_clear_string(option_upper, 0);

  stop_seek=0;
  do
  {
    // On lit une ligne dans le fichier:
    if (fgets(buffer,1024,file)==NULL)
    {
      free(upper_buffer);
      free(option_upper);
      return ERROR_INI_CORRUPTED;
    }

    Line_number_in_INI_file++;

    // On s'en fait une version en majuscule:
    strcpy(upper_buffer,buffer);
    Load_INI_clear_string(upper_buffer, raw_text);

    // On compare la chaîne avec l'option recherchée:
    stop_seek=Load_INI_seek_pattern(upper_buffer,option_upper);

    // Si on l'a trouvée:
    if (stop_seek)
    {
      // On se positionne juste après la chaîne "="
      buffer_index=Load_INI_seek_pattern(upper_buffer,"=");

      strcpy(return_code, upper_buffer + buffer_index);
    }
  }
  while (!stop_seek);

  free(upper_buffer);
  free(option_upper);

  return 0;
}

/**
 * Read a value from the string
 *
 * @ref index is updated according to the number of characters read
 * from the source string.
 *
 * @param str the source string (option "values")
 * @param index a pointer to the current index in the source string
 * @param value the read value will be put there
 * @return 0
 * @return @ref ERROR_INI_CORRUPTED
 */
static int Load_INI_get_value(const char * str,int * index,int * value)
{
  int negative = 0;
  
  // On teste si la valeur actuelle est YES (ou Y):

  if (Load_INI_seek_pattern(str+(*index),"yes,")==1)
  {
    (*value)=1;
    (*index)+=4;
    return 0;
  }
  if (strcmp(str+(*index),"yes")==0)
  {
    (*value)=1;
    (*index)+=3;
    return 0;
  }
  if (Load_INI_seek_pattern(str+(*index),"y,")==1)
  {
    (*value)=1;
    (*index)+=2;
    return 0;
  }
  if (strcmp(str+(*index),"y")==0)
  {
    (*value)=1;
    (*index)+=1;
    return 0;
  }
  
  // On teste si la valeur actuelle est NO (ou N):
  
  if (Load_INI_seek_pattern(str+(*index),"no,")==1)
  {
    (*value)=0;
    (*index)+=3;
    return 0;
  }
  if (strcmp(str+(*index),"no")==0)
  {
    (*value)=0;
    (*index)+=2;
    return 0;
  }
  if (Load_INI_seek_pattern(str+(*index),"n,")==1)
  {
    (*value)=0;
    (*index)+=2;
    return 0;
  }
  if (strcmp(str+(*index),"n")==0)
  {
    (*value)=0;
    (*index)+=1;
    return 0;
  }
  if (str[*index]=='$')
  {
    (*value)=0;

    for (;;)
    {
      (*index)++;

      if ((str[*index]>='0') && (str[*index]<='9'))
        (*value)=((*value)*16)+str[*index]-'0';
      else
      if ((str[*index]>='A') && (str[*index]<='F'))
        (*value)=((*value)*16)+str[*index]-'A'+10;
      else
      if (str[*index]==',')
      {
        (*index)++;
        return 0;
      }
      else
      if (str[*index]=='\0')
        return 0;
      else
        return ERROR_INI_CORRUPTED;
    }
  }
  if (str[*index]=='-')
  {
    negative = 1;
    // next character
    (*index)++;
    // Fall thru
  }
  if ((str[*index]>='0') && (str[*index]<='9'))
  {
    (*value)=0;

    for (;;)
    {
      if ((str[*index]>='0') && (str[*index]<='9'))
      {
        (*value)=((*value)*10)+str[*index]-'0';
        if (negative)
        {
          (*value)*= -1;
          // This is to do it once per number.
          negative = 0;
        }
      }
      else
      if (str[*index]==',')
      {
        (*index)++;
        return 0;
      }
      else
      if (str[*index]=='\0')
        return 0;
      else
        return ERROR_INI_CORRUPTED;

      (*index)++;
    }
  }
  else
    return ERROR_INI_CORRUPTED;
}


/**
 * Read all values from a string
 *
 * The values are comma separated
 *
 * @param file handle to gfx2.ini
 * @param buffer Current text buffer, preserved from one call to the next
 * @param option_name name of the option to read
 * @param nb_expected_values number of values to read from the line
 * @param[out] values the values will be put there
 * @return 0 when OK
 * @return @ref ERROR_INI_CORRUPTED if no value was found, or not enough
 */
static int Load_INI_get_values(FILE * file,char * buffer,const char * option_name,int nb_expected_values,int * values)
{
  int    stop_seek;
  char * option_upper;
  char * upper_buffer;
  int    buffer_index;
  int    nb_values;

  // On alloue les zones de mémoire:
  option_upper=(char *)malloc(1024);
  upper_buffer=(char *)malloc(1024);

  // On commence par se faire une version majuscule de l'option à rechercher:
  strcpy(option_upper,option_name);
  Load_INI_clear_string(option_upper, 0);

  stop_seek=0;
  do
  {
    // On lit une ligne dans le fichier:
    if (fgets(buffer,1024,file)==0)
    {
      free(upper_buffer);
      free(option_upper);
      return ERROR_INI_CORRUPTED;
    }

    Line_number_in_INI_file++;

    // On s'en fait une version en majuscule:
    strcpy(upper_buffer,buffer);
    Load_INI_clear_string(upper_buffer, 0);

    // On compare la chaîne avec l'option recherchée:
    stop_seek=Load_INI_seek_pattern(upper_buffer,option_upper);

    // Si on l'a trouvée:
    if (stop_seek)
    {
      nb_values=0;

      // On se positionne juste après la chaîne "="
      buffer_index=Load_INI_seek_pattern(upper_buffer,"=");

      // Tant qu'on a pas atteint la fin de la ligne
      while (upper_buffer[buffer_index]!='\0')
      {
        if (Load_INI_get_value(upper_buffer,&buffer_index,values+nb_values))
        {
          free(upper_buffer);
          free(option_upper);
          return ERROR_INI_CORRUPTED;
        }

        if ( ((++nb_values) == nb_expected_values) &&
             (upper_buffer[buffer_index]!='\0') )
        {
          // Too many values !
          free(upper_buffer);
          free(option_upper);
          return ERROR_INI_CORRUPTED;
        }
      }

      if (nb_values<nb_expected_values)
      {
        // Not enough values !
        free(upper_buffer);
        free(option_upper);
        return ERROR_INI_CORRUPTED;
      }
    }
  }
  while (!stop_seek);

  free(upper_buffer);
  free(option_upper);

  return 0;
}


/**
 * Load config from the gfx2.ini file
 *
 * @param[out] conf the configuration to read to
 * @return 0 when OK
 * @return @ref ERROR_INI_CORRUPTED for any error
 */
int Load_INI(T_Config * conf)
{
  FILE * file;
  char * buffer;
  int    values[3];
  int    index;
  char * filename;
  int    return_code;
  char   value_label[1024];

  Line_number_in_INI_file=0;
  
#if defined(__WIZ__) || defined(__CAANOO__)
  conf->Stylus_mode = 1;
#else
  conf->Stylus_mode = 0;
#endif

  // allocate buffer
  buffer = (char *)GFX2_malloc(1024);

  filename = Filepath_append_to_dir(Config_directory, INI_FILENAME);
  file = fopen(filename, "r");
  if (file == NULL)
  {
    free(filename);
    // Si le fichier ini est absent on le relit depuis gfx2def.ini
    filename = Filepath_append_to_dir(Data_directory, INIDEF_FILENAME);
    file = fopen(filename, "r");
    if (file == NULL)
    {
      GFX2_Log(GFX2_ERROR, "Load_INI() cannot open %s\n", filename);
      free(filename);
      free(buffer);
      return ERROR_INI_MISSING;
    }
  }
  GFX2_Log(GFX2_DEBUG, "Load_INI() loading %s\n", filename);
  free(filename);
  
  if ((return_code=Load_INI_reach_group(file,buffer,"[MOUSE]")))
    goto Erreur_Retour;

  if ((return_code=Load_INI_get_values (file,buffer,"X_sensitivity",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>4))
    conf->Mouse_sensitivity_index_x=1;
  else
    conf->Mouse_sensitivity_index_x=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Y_sensitivity",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>4))
    conf->Mouse_sensitivity_index_y=1;
  else
    conf->Mouse_sensitivity_index_y=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"X_correction_factor",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>4))
    goto Erreur_ERREUR_INI_CORROMPU;
  // Deprecated setting, unused

  if ((return_code=Load_INI_get_values (file,buffer,"Y_correction_factor",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>4))
    goto Erreur_ERREUR_INI_CORROMPU;
  // Deprecated setting, unused

  if ((return_code=Load_INI_get_values (file,buffer,"Cursor_aspect",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>3))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Cursor=values[0]-1;

  if ((return_code=Load_INI_reach_group(file,buffer,"[MENU]")))
    goto Erreur_Retour;

  conf->Fav_menu_colors[0].R=0;
  conf->Fav_menu_colors[0].G=0;
  conf->Fav_menu_colors[0].B=0;
  conf->Fav_menu_colors[3].R=255;
  conf->Fav_menu_colors[3].G=255;
  conf->Fav_menu_colors[3].B=255;

  if ((return_code=Load_INI_get_values (file,buffer,"Light_color",3,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  if ((values[1]<0) || (values[1]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  if ((values[2]<0) || (values[2]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Fav_menu_colors[2].R=(values[0]<<2)|(values[0]>>4);
  conf->Fav_menu_colors[2].G=(values[1]<<2)|(values[1]>>4);
  conf->Fav_menu_colors[2].B=(values[2]<<2)|(values[2]>>4);

  if ((return_code=Load_INI_get_values (file,buffer,"Dark_color",3,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  if ((values[1]<0) || (values[1]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  if ((values[2]<0) || (values[2]>63))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Fav_menu_colors[1].R=(values[0]<<2)|(values[0]>>4);
  conf->Fav_menu_colors[1].G=(values[1]<<2)|(values[1]>>4);
  conf->Fav_menu_colors[1].B=(values[2]<<2)|(values[2]>>4);

  if ((return_code=Load_INI_get_values (file,buffer,"Menu_ratio",1,values)))
    goto Erreur_Retour;
  if ((values[0]<-4) || (values[0]>2))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Ratio=values[0];

  if ((return_code=Load_INI_reach_group(file,buffer,"[FILE_SELECTOR]")))
    goto Erreur_Retour;

  if ((return_code=Load_INI_get_values (file,buffer,"Show_hidden_files",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Show_hidden_files=values[0]?1:0;

  if ((return_code=Load_INI_get_values (file,buffer,"Show_hidden_directories",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Show_hidden_directories=values[0]?1:0;

/*  if ((return_code=Load_INI_get_values (file,buffer,"Show_system_directories",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Show_system_directories=values[0]?1:0;
*/
  if ((return_code=Load_INI_get_values (file,buffer,"Preview_delay",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>256))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Timer_delay=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Maximize_preview",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Maximize_preview=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Find_file_fast",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>2))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Find_file_fast=values[0];


  if ((return_code=Load_INI_reach_group(file,buffer,"[LOADING]")))
    goto Erreur_Retour;

  if ((return_code=Load_INI_get_values (file,buffer,"Auto_set_resolution",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Auto_set_res=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Set_resolution_according_to",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>2))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Set_resolution_according_to=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Clear_palette",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Clear_palette=values[0];


  if ((return_code=Load_INI_reach_group(file,buffer,"[MISCELLANEOUS]")))
    goto Erreur_Retour;

  if ((return_code=Load_INI_get_values (file,buffer,"Draw_limits",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Display_image_limits=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Adjust_brush_pick",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Adjust_brush_pick=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Coordinates",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>2))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Coords_rel=2-values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Backup",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Backup=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Undo_pages",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>99))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Max_undo_pages=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Gauges_scrolling_speed_Left",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>255))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Delay_left_click_on_slider=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Gauges_scrolling_speed_Right",1,values)))
    goto Erreur_Retour;
  if ((values[0]<1) || (values[0]>255))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Delay_right_click_on_slider=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Auto_save",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Auto_save=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Vertices_per_polygon",1,values)))
    goto Erreur_Retour;
  if ((values[0]<2) || (values[0]>16384))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Nb_max_vertices_per_polygon=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Fast_zoom",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Fast_zoom=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Separate_colors",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Separate_colors=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"FX_feedback",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->FX_Feedback=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Safety_colors",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Safety_colors=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Opening_message",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Opening_message=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Clear_with_stencil",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Clear_with_stencil=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Auto_discontinuous",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Auto_discontinuous=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Save_screen_size_in_GIF",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Screen_size_in_GIF=values[0];

  if ((return_code=Load_INI_get_values (file,buffer,"Auto_nb_colors_used",1,values)))
    goto Erreur_Retour;
  if ((values[0]<0) || (values[0]>1))
    goto Erreur_ERREUR_INI_CORROMPU;
  conf->Auto_nb_used=values[0];

  // Optionnel, le mode video par défaut (à partir de beta 97.0%)
  conf->Default_resolution=0;
  if (!Load_INI_get_string (file,buffer,"Default_video_mode",value_label, 0))
  {
    int mode = Convert_videomode_arg(value_label);
    if (mode>=0)
      conf->Default_resolution=mode;
  }
  
  // Optionnel, les dimensions de la fenêtre (à partir de beta 97.0%)
  // Do that only if the first mode is actually windowed (not the case on gp2x for example)
  if(Video_mode[0].Fullscreen==0)
  {
    Video_mode[0].Width = 640;
    Video_mode[0].Height = 480;
    if (!Load_INI_get_values (file,buffer,"Default_window_size",2,values))
    {
      if ((values[0]>=320))
        Default_window_width = Video_mode[0].Width = values[0];
      if ((values[1]>=200))
        Default_window_height = Video_mode[0].Height = values[1];
    }
  }

  conf->Mouse_merge_movement=100;
  // Optionnel, paramètre pour grouper les mouvements souris (>98.0%)
  if (!Load_INI_get_values (file,buffer,"Merge_movement",1,values))
  {
    if ((values[0]<0) || (values[0]>1000))
      goto Erreur_ERREUR_INI_CORROMPU;
    conf->Mouse_merge_movement=values[0];
  }

  conf->Mouse_motion_debounce=0;
  // Optional: debounce mouse motion handling by a given number of milliseconds
  if (!Load_INI_get_values( file, buffer, "Mouse_motion_debounce",1,values))
  {
    if (values[0] < 0 || values[0] > 1000)
      goto Erreur_ERREUR_INI_CORROMPU;
    conf->Mouse_motion_debounce=values[0];
  }

  conf->Palette_cells_X=16;
  // Optionnel, nombre de colonnes dans la palette (>98.0%)
  if (!Load_INI_get_values (file,buffer,"Palette_cells_X",1,values))
  {
    if ((values[0]<1) || (values[0]>256))
      goto Erreur_ERREUR_INI_CORROMPU;
    conf->Palette_cells_X=values[0];
  }
  conf->Palette_cells_Y=4;
  // Optionnel, nombre de lignes dans la palette (>98.0%)
  if (!Load_INI_get_values (file,buffer,"Palette_cells_Y",1,values))
  {
    if (values[0]<1 || values[0]>16)
      goto Erreur_ERREUR_INI_CORROMPU;
    conf->Palette_cells_Y=values[0];
  }
  // Optionnel, bookmarks (>98.0%)
  for (index=0;index<NB_BOOKMARKS;index++)
  {
    conf->Bookmark_directory[index]=NULL;
    conf->Bookmark_label[index][0]='\0';  
  }
  for (index=0;index<NB_BOOKMARKS;index++)
  {
    if (!Load_INI_get_string (file,buffer,"Bookmark_label",value_label, 1))
    {
      size_t size = strlen(value_label);
      if (size!=0)
      {
        if (size >= sizeof(conf->Bookmark_label[0]))
        {
          memcpy(conf->Bookmark_label[index], value_label, sizeof(conf->Bookmark_label[0]) - 1);
          conf->Bookmark_label[index][sizeof(conf->Bookmark_label[0]) - 1] = '\0';
        }
        else
          memcpy(conf->Bookmark_label[index], value_label, size + 1);
      }
    }
    else
      break;
    if (!Load_INI_get_string (file,buffer,"Bookmark_directory",value_label, 1))
    {
      size_t size = strlen(value_label);
      if (size!=0)
      {
        conf->Bookmark_directory[index]=(char *)malloc(size+1);
        strcpy(conf->Bookmark_directory[index],value_label);
      }
    }
    else
      break;
  }
  conf->Palette_vertical=1;
  // Optional, vertical palette option (>98.0%)
  if (!Load_INI_get_values (file,buffer,"Palette_vertical",1,values))
  {
    if ((values[0]<0) || (values[0]>1))
      goto Erreur_ERREUR_INI_CORROMPU;
    conf->Palette_vertical=values[0];
  }
  
  // Optional, the window position (>98.0%)
  conf->Window_pos_x=9999;
  conf->Window_pos_y=9999;
  if (!Load_INI_get_values (file,buffer,"Window_position",2,values))
  {
    conf->Window_pos_x = values[0];
    conf->Window_pos_y = values[1];
  }
  
  conf->Double_click_speed=500;
  // Optional, speed of double-click (>2.0)
  if (!Load_INI_get_values (file,buffer,"Double_click_speed",1,values))
  {
    if ((values[0]>0) || (values[0]<=2000))
      conf->Double_click_speed=values[0];
  }

  conf->Double_key_speed=500;
  // Optional, speed of double-keypress (>2.0)
  if (!Load_INI_get_values (file,buffer,"Double_key_speed",1,values))
  {
    if ((values[0]>0) || (values[0]<=2000))
      conf->Double_key_speed=values[0];
  }

  // Optional, name of skin file. (>2.0)
  if(!Load_INI_get_string(file,buffer,"Skin_file",value_label,1))
  {
    conf->Skin_file = strdup(value_label);
  }
  else
    conf->Skin_file = strdup(DEFAULT_SKIN_FILENAME);

  // Optional, name of font file. (>2.0)
  if(!Load_INI_get_string(file,buffer,"Font_file",value_label,1))
    conf->Font_file = strdup(value_label);
  else
    conf->Font_file = strdup(DEFAULT_FONT_FILENAME);

  // Optional, "fake hardware zoom" factor (>2.1)
  if (!Load_INI_get_values (file, buffer,"Pixel_ratio",1,values))
  {
    Pixel_ratio = values[0];
    switch(Pixel_ratio) {
      case PIXEL_WIDE:
        if(Video_mode[0].Width < 640)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_TALL:
        if(Video_mode[0].Height < 400)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_DOUBLE:
        if(Video_mode[0].Width < 640 || Video_mode[0].Height < 400)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_TRIPLE:
        if(Video_mode[0].Width < 3*320 || Video_mode[0].Height < 3*200)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_WIDE2:
        if(Video_mode[0].Width < 4*320 || Video_mode[0].Height < 2*200)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_TALL2:
        if(Video_mode[0].Width < 2*320 || Video_mode[0].Height < 4*200)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_TALL3:
        if(Video_mode[0].Width < 3*320 || Video_mode[0].Height < 4*200)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      case PIXEL_QUAD:
        if(Video_mode[0].Width < 4*320 || Video_mode[0].Height < 4*200)
          Pixel_ratio = PIXEL_SIMPLE;
        break;
      default:
        // Convert back unknown values to PIXEL_SIMPLE
        Pixel_ratio = PIXEL_SIMPLE;
        break;
    }
  }
  
  // Optional, Menu bars visibility (> 2.1)
  if (!Load_INI_get_values (file, buffer,"Menubars_visible",1,values))
  {
    byte anim_visible = (values[0] & 2)!=0;
    byte tools_visible = (values[0] & 4)!=0;
    
    // Skip status bar, always enabled.
    Menu_bars[MENUBAR_LAYERS].Visible = anim_visible;
    Menu_bars[MENUBAR_ANIMATION].Visible = 0;
    Menu_bars[MENUBAR_TOOLS].Visible = tools_visible;
  }
  
  conf->Right_click_colorpick=0;
  // Optional, right mouse button to pick colors (>=2.3)
  if (!Load_INI_get_values (file,buffer,"Right_click_colorpick",1,values))
  {
    conf->Right_click_colorpick=(values[0]!=0);
  }
  
  conf->Sync_views=1;
  // Optional, synced view of main and spare (>=2.3)
  if (!Load_INI_get_values (file,buffer,"Sync_views",1,values))
  {
    conf->Sync_views=(values[0]!=0);
  }
  
  conf->Swap_buttons=0;
  // Optional, key for swap buttons (>=2.3)
  if (!Load_INI_get_values (file,buffer,"Swap_buttons",1,values))
  {
    switch(values[0])
    {
      case 1:
        conf->Swap_buttons=GFX2_MOD_CTRL;
        break;
      case 2:
        conf->Swap_buttons=GFX2_MOD_ALT;
        break;
    }
  }
  
  // Optional, Location of last directory used for Lua scripts browsing (>=2.3)
  free(conf->Scripts_directory);
  conf->Scripts_directory = NULL;
  if (!Load_INI_get_string (file,buffer,"Scripts_directory",value_label, 1))
  {
    if (value_label[0] != '\0')
      conf->Scripts_directory = strdup(value_label);
  }
  if (conf->Scripts_directory == NULL)
  {
    // Default when empty:
    char * path = Realpath(Data_directory);
    conf->Scripts_directory = Filepath_append_to_dir(path, SCRIPTS_SUBDIRECTORY);
    free(path);
  }
  
  conf->Allow_multi_shortcuts=0;
  // Optional, allow or disallow multiple shortcuts on same key (>=2.3)
  if (!Load_INI_get_values (file,buffer,"Allow_multi_shortcuts",1,values))
  {
    conf->Allow_multi_shortcuts=(values[0]!=0);
  }
  
  conf->Tilemap_allow_flipped_x=0;
  // Optional, makes tilemap effect detect x-flipped tiles (>=2.4)
  if (!Load_INI_get_values (file,buffer,"Tilemap_detect_mirrored_x",1,values))
  {
    conf->Tilemap_allow_flipped_x=(values[0]!=0);
  }
  
  conf->Tilemap_allow_flipped_y=0;
  // Optional, makes tilemap effect detect y-flipped tiles (>=2.4)
  if (!Load_INI_get_values (file,buffer,"Tilemap_detect_mirrored_y",1,values))
  {
    conf->Tilemap_allow_flipped_y=(values[0]!=0);
  }
  
  conf->Tilemap_show_count=0;
  // Optional, makes tilemap effect display tile count (>=2.4)
  if (!Load_INI_get_values (file,buffer,"Tilemap_count",1,values))
  {
    conf->Tilemap_show_count=(values[0]!=0);
  }
  
  conf->Use_virtual_keyboard=0;
  // Optional, enables virtual keyboard (>=2.4)
  if (!Load_INI_get_values (file,buffer,"Use_virtual_keyboard",1,values))
  {
    if (values[0]>=0 && values[0]<=2)
      conf->Use_virtual_keyboard=values[0];
  }

  conf->Default_mode_layers=0;
  // Optional, remembers if the user last chose layers or anim (>=2.4)
  if (!Load_INI_get_values (file,buffer,"Default_mode_layers",1,values))
  {
    conf->Default_mode_layers=(values[0]!=0);
  }

  conf->MOTO_gamma=28;
  // Optional, gamma value used for palette of load/save Thomson MO/TO pictures (>=2.6)
  if (!Load_INI_get_values (file,buffer,"MOTO_gamma",1,values))
  {
    conf->MOTO_gamma=(byte)values[0];
  }
  
  // Insert new values here

  fclose(file);

  free(buffer);
  return 0;

  // Gestion des erreurs:

  Erreur_Retour:
    fclose(file);
    free(buffer);
    return return_code;

  Erreur_ERREUR_INI_CORROMPU:

    fclose(file);
    free(buffer);
    return ERROR_INI_CORRUPTED;
}
