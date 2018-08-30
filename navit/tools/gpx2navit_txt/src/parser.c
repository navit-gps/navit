/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "gpx2navit_txt.h"

void charHandle(void *userdata, const XML_Char * data, int length);
void startElement(void *userdata, const char *element, const char **attr);
void endElement(void *userdata, const char *element);
void parseMain(g2sprop * prop);

/**
 * a handler to parse charctor data on expat
 */
void charHandle(void *userdata, const XML_Char * data, int length) {
    static int bufsize = DATABUFSIZE;
    static int string_length = 0;
    int new_length;
    static int begin_copy = 0;
    int i;
    parsedata *pdata = (parsedata *) userdata;
    if (pdata->bufptr == NULL) {
        //start of buffer -->pdata->bufptr set to 0 at endelement
        string_length = 0;
        begin_copy = 0; //begin to copy after first space
        pdata->bufptr= pdata->databuf;
    }
    new_length = string_length + length + 1; //additonal 0
    if (bufsize < new_length) {
        pdata->databuf =
            realloc(pdata->databuf, new_length);
        bufsize = new_length;
        //because of realloc the pointer may have changed
        pdata->bufptr = pdata->databuf + string_length;
    }
    // because expat calls this routine several times on special chars
    // we need to do following
    // --concat strings until reset (bufptr set to NULL)
    // --filter out blank chars at begin of string
    for (i=0; i<length; i++) {
        if (begin_copy || !isspace(data[i])) {
            *pdata->bufptr = data[i];
            pdata->bufptr++;
            string_length ++;
            begin_copy = 1;
            if (DEBUG) fprintf(stderr,"%c",data[i]);
        }
    }
    *pdata->bufptr = '\0';
}

/**
 * a handler when a element starts
 */
void startElement(void *userdata, const char *element, const char **attr) {
    parsedata *pdata = (parsedata *) userdata;
    pdata->parent = pdata->current;
    pdata->current = malloc(sizeof(parent));
    pdata->current->name = malloc(sizeof(char) * (strlen(element) + 1));
    strcpy(pdata->current->name, element);
    pdata->current->parentptr = pdata->parent;
    startElementControl(pdata, element, attr);
    if (pdata->prop->verbose) {
        int i;
        for (i = 0; i < pdata->depth; i++)
            printf("  ");
        printf("<%s>: ", element);
        for (i = 0; attr[i]; i += 2) {
            printf(" %s='%s'", attr[i], attr[i + 1]);
        }
        printf("\n");
    }
    pdata->depth++;
}

/**
 * a handler when a element ends
 */
void endElement(void *userdata, const char *element) {
    parsedata *pdata = (parsedata *) userdata;
    endElementControl(pdata, element);
    pdata->depth--;
    if (pdata->prop->verbose) {
        int i;
        for (i = 0; i < pdata->depth; i++)
            printf("  ");
        printf("</%s>:%s\n ", element,pdata->parent->name);
    }
    free(pdata->current->name);
    free(pdata->current);
    pdata->current = pdata->parent;
    pdata->parent = pdata->parent->parentptr;
}

void parseMain(g2sprop * prop) {
    FILE *fp;
    char buff[BUFFSIZE];
    XML_Parser parser;
    parsedata *pdata;
    fp = fopen(prop->sourcefile, "r");
    if (fp == NULL) {
        fprintf(stderr, "Cannot open gpx file: %s\n", prop->sourcefile);
        exit(ERR_CANNOTOPEN);
    }
    parser = XML_ParserCreate(NULL);
    if (!parser) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(ERR_OUTOFMEMORY);
    }
    pdata = createParsedata(parser, prop);

    char *output_wpt =
        (char *) malloc(sizeof(char) * (strlen(pdata->prop->output) + 9));
    strcpy(output_wpt, pdata->prop->output);
    strcat(output_wpt, "_nav.txt");
    pdata->fp = fopen(output_wpt,"w");
    if (pdata->fp == NULL) {
        //todo
        fprintf(stderr,"Failure opening File %s for writing",output_wpt);
        exit(1);
    }
    free(output_wpt);
    XML_SetUserData(parser, pdata);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, charHandle);
    for (;;) {
        int done;
        int len;
        fgets(buff, BUFFSIZE, fp);
        len = (int) strlen(buff);
        if (ferror(fp)) {
            fprintf(stderr, "Read error file: %s\n", prop->sourcefile);
            exit(ERR_READERROR);
        }
        done = feof(fp);
        if (done)
            break;
        if (!XML_Parse(parser, buff, len, done)) {
            fprintf(stderr, "Parse error at line %d:\n%s\n",
                    XML_GetCurrentLineNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
            exit(ERR_PARSEERROR);
        }
    }
    fclose(pdata->fp); //close out file
    closeParsedata(pdata);
}
