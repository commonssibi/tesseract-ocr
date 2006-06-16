/**************************************************************************

    $Log$
    Revision 1.1  2006/06/16 22:17:05  lvincent
    Initial checkin of Tesseract 1.0

    Revision 1.1.1.1  2004/02/20 19:38:50  slumos
    Import original HP distribution

* Revision 5.1  89/07/27  11:46:39  11:46:39  ray ()
* Added ratings acces methods.
* This version ready for independent development.
*

**************************************************************************/
#include <string.h>
#include <ctype.h>

/*chartoname(name,c,dir) converts c into a useful filename*/
void chartoname(register char *name,  /*result */
                char c,               /*char to convert */
                const char *dir) {    /*directory to use */
  char file[3];                  /*filename */
  int index;                     /*index of namelist */
  static const char *namelist[] = {
    "!bang",
    "\"doubleq",
    "#hash",
    "$dollar",
    "%percent",
    "&and",
    "'quote",
    "(lround",
    ")rround",
    "*asterisk",
    "+plus",
    ",comma",
    "-minus",
    ".dot",
    "/slash",
    ":colon",
    ";semic",
    "<less",
    "=equal",
    ">greater",
    "?question",
    "@at",
    "[lsquare",
    "\\backsl",
    "]rsquare",
    "^uparr",
    "_unders",
    "`grave",
    "{lbrace",
    "|bar",
    "}rbrace",
    "~tilde"
  };

  strcpy(name, dir);  /*add specific directory */
  for (index = 0; index < sizeof namelist / sizeof (char *)
    && c != namelist[index][0]; index++);
  if (index < sizeof namelist / sizeof (char *))
                                 /*add text name */
    strcat (name, &namelist[index][1]);
  else {
    if (isupper (c)) {
      file[0] = 'c';             /*direct a-z or A-Z */
      file[1] = c;               /*direct a-z or A-Z */
      file[2] = '\0';
    }
    else {
      file[0] = c;               /*direct a-z or A-Z */
      file[1] = '\0';
    }
    strcat(name, file);  /*append filename */
  }
}
