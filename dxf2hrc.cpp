/*************************** DFX2HRC.C ****************************
   Translator DXF format to SOFTIMAGE format (.hrc), v1.3
   Reading DXF-file from AutoDESK 3D STUDIO (only for tri-mesh).
   Writing HRC-file for SOFTIMAGE Creative Environment Inc.
   (only for PMESH type object).

   usage:     dxf2hrc.exe  file_dxf  [file_hrc]  [options]
   options:   -h, help, ?  -->  Help
	      -l,          -->  Output list Vertex and Faces

   Writing by S.Gasanov
   Ukrainian, Kiev, Manufacture of Advertisement Co.
   tel/fax: (044) 295-94-87

   Date create original:  05.09.94
   Last modification:     29.09.94
   Version for BORLAND C++ (BC) compilator (20.09.94)
 
 *----------------------------------------------------------------
 * Version for Visual Studio 2019 
 * Modified by sgiman @ 2022
 *-----------------------------------------------------------------*/

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hrcheadr.h"
#include "hrcstruc.h"
#include "utils.h"

using namespace std;

/*-------------- Constant parameters ---------------------
   Value parameter PROCESS (Mode Atomatic Discontinuity):
   0C (04) - Automatic/Process=On  - with smoothing object
   08 (00) - Automatic/Process=Off - with smoothing object
   0E (06) - Faceted/Process=On    - analog 3DS facet object
   0A (02) - Faceted/Process=Off   - analog 3DS facet object

   Value parameter CODE:
   04 - Polygonal mesh (PMESH)
----------------------------------------------------------*/

static char title[8] = { 0, 0, 0, 0,'\H','\R','\C','\H' }; /* Prefix name */
#define  CODE       0x04   /* Code object - PMESH (04) */
#define  PROCESS    0x0C   /* Mode smoothing process (Faceted/Process=On)*/
#define  ANGLE      60.0   /* Value smoothing */
#define  ZERO       0L     /* Zero marker (4 bytes = 0) */
#define  XAXIS      1.0    /* X center object */
#define  YAXIS      1.0    /* Y center object */
#define  ZAXIS      1.0    /* Z center object */
#define  SIZE       80     /* Length input dxf-string */
#define  LINK       0x80   /* Marker for objects > 1 */
#define  BUFSIZE    30000  /* Size buffers */

/*-------------- Variaty parameteres ------------------*/
/* Constants */
#define  VFACE      3           /* Number vertices to face */
#define  MEDGE      0x1080      /* Dump = (80 10) or (80 00) */
#define  XOBJCEN    0.0         /* Coordinate X for center object */
#define  YOBJCEN    0.0         /* Coordinate Y for center object */
#define  ZOBJCEN    0.0         /* Coordinate Z for center object */
#define  NEDGE      24          /* Number EDGES */

/* Varieties */
static char   Name[SIZE]; /* Name object */
static long   Nvert;      /* Number vertices POLYMESH */
static long   Nface;      /* Number faces POLYMESH */
static long   Nedge;      /* Number edges POLYMESH */
static short  Nobj;       /* Number objects */
static float  x_vert, y_vert, z_vert;
static float  x_norm, y_norm, z_norm;
static long   v1, v2, v3;
static int    flag, err;
static char   s_tag[SIZE], s_val[SIZE], cs[SIZE];
static long   nv[VFACE];
static int    next_s = 0;
static int    flist = 0;

/* User functions */
void    conv_process(void);
void    blocks(void);
void    entities(void);
void    conv_polyline(void);
void    conv_vertex(void);
void    conv_faces(void);
void    conv_edges(void);
void    read_polyline(void);
void    write_name_inf(void);
void    write_endmesh(void);
void    write_cen_axis(void);
void    ferr(size_t, size_t);
char    *read2(void);
double  atoff(char*);
void    strcopy(char*, char*);
void    swap(char*, unsigned n);
void    fexists(char*);

/***** void    def_hrc_name(char *); ***/
void    control_dxf(void);
void    write_null(void);
void    msg_title(void);

static  char* file_hrc, * file_dxf;
static  fpos_t* pos;
static  FILE* dxf, * hrc;  /* Descriptors working files */

static char buf_dxf[BUFSIZE], buf_hrc[BUFSIZE];

/*===========================
           M A I N
============================*/
int main(int argc, char* argv[])
{
    /* --- TIME PAUSE --- */
    cout << endl;
    MyDate();
    system("pause");
   
    // --- CONVERT DXF TO HRC (Softimage 3.x) ----     
    int i;
    file_dxf = (char*)malloc(SIZE);
    file_hrc = (char*)malloc(SIZE);

    /* Testing arguments */
    if (argc < 2)  msg_title();    /* If not arguments */
    for (i = 1; i <= argc; i++)
    {
        switch (argv[i][0])
        {
        case '-':
            if (memcmp(argv[i], "-h", 2) == 0)  /* if help */
            {
                msg_title();
                break;
            }
            if (memcmp(argv[i], "-l", 2) == 0)   /* If list output */
            {
                flist = 1;
                break;
            }
        case 'h':
        case '?':
            if ((memcmp(argv[i], "help", 4) == 0) ||  /* If help */
                (memcmp(argv[i], "?", 1) == 0))
            {
                msg_title();
                break;
            }
        default:    break;

        } /* end switch */
    } /* end for */

    /*------- Open 3DS DXF file --------*/
    if (memcmp(argv[1], "-l", 2) == 0)
        *file_dxf = NULL;
    else
        file_dxf = argv[1];

    if ((dxf = fopen(file_dxf, "r")) == NULL)
    {
        fprintf(stderr, "\nCan't open DXF file for reading...  %s\n", file_dxf);
        exit(1);
    }
    control_dxf();

    /*------- Open SOFTIMAGE file (.HRC) --------*/
    if (memcmp(argv[2], "-l", 2) == 0)
        exit(1);
    /*	def_hrc_name(file_dxf); */
    else
        strcpy(file_hrc, argv[2]);

    if (argv[2] == NULL)
        /* def_hrc_name(file_dxf); */
        exit(1);
    fexists(file_hrc);

    if ((hrc = fopen(file_hrc, "ab")) == NULL)
    {
        fprintf(stderr, "\nCan't open HRC file for writing: %s\n", file_hrc);
        exit(2);
    }

    /*------------- Buffers for data ------------*/
    setvbuf(dxf, buf_dxf, _IOFBF, BUFSIZE);   /* For reading data from DXF */
    setvbuf(hrc, buf_hrc, _IOFBF, BUFSIZE);   /* For writing data to HRC */

    /*---------- Writing heder to hrc-file -------------*/
    err = fwrite(&data_header, sizeof(data_header), 1, hrc);
    ferr(err, 1);

    /*-----------------------------------------------------------
                Converting process
    ------------------------------------------------------------*/
    fprintf(stderr, "\nPlease wait....\n");
    while (read2() != NULL)
    {
        if (memcmp(s_val, "SECTION", 6) == 0)
        {
            read2();
            if (memcmp(s_val, "TABLES", 6) == 0)
                conv_process();
        }
    }

    fprintf(stderr, "\nNot found TABLE \"LAYER\"...\n");
    exit(-1);

} // *****  E N D   M A I N  ******


/*-----------------------------
        conv_process()
------------------------------*/
void conv_process(void)
{
    read2();  /* Tag = 0, Value = "TABLE" */
    read2();  /* Tag = 2, Value = name table */

    while (read2() != NULL)
    {
        if (memcmp(s_val, "LAYER", 5) != 0)
            continue;
        break;
    };

    Nobj = atoi(read2());     				/* Tag = 70, Value = number objects */

    /* Finding sections "BLOCKS" or "ENTITIES" */
    while (read2() != NULL)
    {
        if (memcmp(s_val, "SECTION", 7) == 0)    	/* if s_val = "SECTION" */
        {
            read2();                               	/* Enter Tag, Value */
            if (memcmp(s_val, "BLOCKS", 6) == 0)
                blocks();
            if (memcmp(s_val, "ENTITIES", 8) == 0)
                entities();
        }
    }
}

/*-----------------------
        entities()
-------------------------*/
void entities(void)
{
    int i;

    /* Writing NULL object */
    write_null();
    write_endmesh();

    /* Finding: Tag=0, Value="POLYLINE" */
    while (memcmp(s_val, "POLYLINE", 8) != 0)
        read2();

    /* Converting POLYMESH */
    for (i = 1; i <= Nobj; i++)
    {
        read_polyline();
        write_name_inf();
        conv_vertex();
        conv_faces();
        conv_edges();
        write_cen_axis();
        write_endmesh();
    };
}

/*----------------------- 
         read2() 
------------------------*/
char* read2(void)
{
    fgets(s_tag, SIZE, dxf);    /* Tag */
    fgets(s_val, SIZE, dxf);    /* Value */
    if (memcmp(s_tag, "999", 3) == 0)  /* Passing Tag = 999 - Coment */
    {
        printf("\nFind comment: %s", s_val);
        fgets(s_tag, SIZE, dxf);  /* Tag */
        fgets(s_val, SIZE, dxf);  /* Value */
    }
    if (memcmp(s_val, "EOF", 3) == 0 || s_val == NULL)  /* End DXF-file */
    {
        printf("\nConverting Objects: %d", next_s);
        printf("\nEnd of DXF file\n");
        fprintf(stderr, "\nConverting complete...\n");
        fclose(dxf);
        fclose(hrc);
        exit(0);
    }

    return ((char*)s_val);
}

/*------------------------ 
   (7) write_endmesh() 
-------------------------*/
void write_endmesh()
{
    short  short_end;  /* Marker for end DATA all meshes */
    short  group_end;
    long   long_end, zero_end;
    struct s_endmesh endmesh;

    zero_end = 0L;
    group_end = 0;
    short_end = 1;
    swap((char*)&short_end, sizeof(short_end));
    long_end = 1L;
    swap((char*)&long_end, sizeof(long_end));

    /*********** (7) Writing END DATA mesh (48 bytes) ***********/
    endmesh.zero1 = ZERO;          /* word 1 */
    endmesh.mode = LINK;          /* word 2 */
    endmesh.zero3 = ZERO;          /* word 3 */
    endmesh.x_obj_cen = XOBJCEN;   /* word 4 */
    endmesh.y_obj_cen = YOBJCEN;   /* word 5 */
    endmesh.z_obj_cen = ZOBJCEN;   /* word 6 */
    endmesh.zero7 = ZERO;          /* word 7 */
    endmesh.zero8 = ZERO;          /* word 8 */
    endmesh.zero9 = ZERO;          /* word 9 */
    endmesh.zero10 = ZERO;         /* word 10 */
    endmesh.zero11 = ZERO;         /* word 11 */

    err = fwrite(&endmesh, sizeof(endmesh), 1, hrc);
    ferr(err, 1);
    switch (next_s)
    {
    case 0:
        err = fwrite(&short_end, sizeof(short_end), 1, hrc);
        ferr(err, 1);
        next_s += 1;
        break;
    default:
        if (next_s == Nobj)
        {
            err = fwrite(&zero_end, sizeof(zero_end), 1, hrc);
            ferr(err, 1);
            err = fwrite(&group_end, sizeof(group_end), 1, hrc);
            ferr(err, 1);
        }
        else
        {
            err = fwrite(&long_end, sizeof(long_end), 1, hrc);
            ferr(err, 1);
            next_s += 1;
        }
    }

    if (next_s > 1)
    {
        read2();    /* Tag = 0,  Value = "SEQEND"  */
        read2();    /* Tag = 8,  Value = name layer */
        read2();    /* Tag = 0,  Value = "POLYLINE" (0) */
    }

    /*....................................
    printf("\nEND: TAG = %s",s_tag);
    printf("\nEND: VAL = %s",s_val);
    printf("\nEND: NEXT = %d",next_s);
    ......................................*/
}

/*--------------------- 
    read_polyline() 
----------------------*/
void read_polyline()
{
    read2();                                    /* Name object, Tag=8, Value=Name Object */
    strcopy(Name, s_val);
    read2();                                    /* Tag=66, Value=1 */
    if (atoi(s_val) == 1)                       /* if next_s atributes */
        if (atoi(read2()) == 64)                /* If Tag=70, Value=64 */
        {
            Nvert = (long)atoi(read2());        /* Number vertix, Tag=71, Value=8 */
            Nface = (long)atoi(read2());        /* Number faces, Tag=72, Value=12 */
            if (next_s == 1) {
                puts("  *********************************************");
                puts("  *        TRANSLATOR 3DS DXF-FILE TO         *");
                puts("  *            SOFTIMAGE HRC-FILE             *");
                puts("  *               (version 1.3)               *");
                puts("  *********************************************");
                printf("  (C) Copyright by S.Gasanov, September, 1994\n");
                printf("  Ukrainian, Kiev, Manufacture of Advertisement\n");
                printf("  tel/fax: (044) 295-94-87\n\n");

                printf("  Objects (type trimesh): %d\n", Nobj);
            }

            /****** (1,2) NAME and INFO MESH ******/
            printf("\n  +++++++++++++++++++++++++++++++++++++++++");
            printf("\n                 \"%s\"\n", Name);
            printf("  +++++++++++++++++++++++++++++++++++++++++\n");
            printf("  Code object:\t\t%d (Poly-mesh)", CODE);
            printf("\n  Name object:\t\t\"%s\"", Name);
            printf("\n  Vertices:\t\t%d", Nvert);
            printf("\n  Faces:\t\t%d", Nface);
            printf("\n  Smoothing angle:\t%f", ANGLE);
        }
        else
        {
            fprintf(stderr, "\nDXF: Object \"%s\" is no POLY-MESH from AutoDESK 3D STUDIO.", Name);
            exit(1);
        }
}

/*---------------------- 
    (1) write_null 
-----------------------*/
void write_null(void)
{
    char dev[4], dir[70], file[10], suff[5];
    struct s_infonull infonull;

    infonull.code = 0;          /* Code object NULL */
    infonull.mode = 0;          /* Mode object NULL */

    infonull.xnull = 1;          /* X coordinate */
    swap((char*)&infonull.xnull, sizeof(infonull.xnull));
    infonull.ynull = 1;          /* Y coordinate */
    swap((char*)&infonull.ynull, sizeof(infonull.ynull));
    infonull.znull = 1;          /* Z coordinate */
    swap((char*)&infonull.znull, sizeof(infonull.znull));

    /* Write title to HRC file = 00,00,00,00,'H','R','C','H' ... */
    if (next_s == 0) {
        err = fwrite(title, 1, 8, hrc);
        ferr(err, 8);
    }

 /* Write name & info NULL objec to HRC file ?????????????????? .....*/
 /************************************************************************
    fnsplit(file_hrc,dev,dir,file,suff);
    err=fwrite (file, (strlen(file) + 1), 1, hrc);
    ferr(err,1);
    err=fwrite (&infonull, sizeof(infonull), 1, hrc);
    ferr(err,1);
 *************************************************************************/
}

/*------------------------- 
    (2) write_name_inf 
--------------------------*/
void write_name_inf()
{
    struct s_info info;

    info.type = CODE;
    swap((char*)&info.type, sizeof(info.type));
    info.mod_smooth = PROCESS;
    swap((char*)&info.mod_smooth, sizeof(info.mod_smooth));
    info.val_smooth = ANGLE;
    swap((char*)&info.val_smooth, sizeof(info.val_smooth));
    info.vertices = Nvert;
    swap((char*)&info.vertices, sizeof(info.vertices));

    err = fwrite(Name, (strlen(Name) + 1), 1, hrc);
    ferr(err, 1);
    err = fwrite(&info, sizeof(info), 1, hrc);
    ferr(err, 1);
}

/*----------------------- 
      conv_vertex() 
------------------------*/
void conv_vertex(void)
{
    int i;
    struct s_vertex vertex;

    printf("\n  Converting verticies...");
    for (i = 0; i < Nvert; i++)
    {
        read2();                                 /* Tag=0, Val="VERTEX" */
        if (memcmp(s_val, "VERTEX", 6) == 0)
        {
            read2();                            /* Passing Tag=8, Value=Name_layer */
            read2();                            /* Tag = 10, Value = X */
            x_vert = atoff(s_val);
            read2();                            /* Tag = 20, Value = Y */
            y_vert = atoff(s_val);
            read2();                            /* Tag = 30, Value = Z */
            z_vert = atoff(s_val);

            if (flist)
            {
                printf("\n   X[%d]= %f", i, x_vert);
                printf("   Y[%d]= %f", i, y_vert);
                printf("   Z[%d]= %f", i, z_vert);
            }

            /*--------- writing vertex ---------*/
            vertex.x_coord = x_vert;
            swap((char*)&vertex.x_coord, sizeof(vertex.x_coord));
            vertex.y_coord = y_vert;
            swap((char*)&vertex.y_coord, sizeof(vertex.y_coord));
            vertex.z_coord = z_vert;
            swap((char*)&vertex.z_coord, sizeof(vertex.z_coord));
            vertex.zero = (int)ZERO;

            err = fwrite(&vertex, sizeof(vertex), 1, hrc);
            ferr(err, 1);

            flag = atoi(read2());       /* Flag vertex POLYMESH,Tag=70,Value=192 */
            if (flag != 192 && i < Nvert)
            {
                fprintf(stderr, "\nDXF: Invalid structure Vertix-%d", i);
                exit(-2);
            }
        }; /* end if */
    }; /* end for */

    /* Saving current position for start vertex of faces to DXF */
    if (fgetpos(dxf, pos) != 0)
    {
        fprintf(stderr, "\nError writing DXF (pass 1)...");
        exit(1);
    }
}

/*--------------------- 
      conv_faces() 
----------------------*/
void conv_faces(void)
{
    int   i, j;
    short vert_face1;    /* Vertex to face (4,3) for begin */
    long  vert_face;     /* Vertex to face (4,3) for next_s */
    long  num_face;
    struct s_face face;

    /* Writing number faces to polymesh object */
    num_face = Nface;
    swap((char*)&num_face, sizeof(num_face));
    err = fwrite(&num_face, sizeof(num_face), 1, hrc);
    ferr(err, 1);

    Nedge = 0;
    printf("\n  Converting faces...");

    for (i = 0; i < Nface; i++)                 /* for -1 */
    {
        read2();                                /*   Tag=0, Val="VERTEX" */
        if (memcmp(s_val, "VERTEX", 6) == 0)    /* if-1 */
        {
            read2();                            /* Passing Tag=8, Value=Name_layer */
            read2();                            /* Tag=10, Value=Xn=0 */
            x_norm = atoff(s_val);
            read2();                            /* Tag=20, Value=Yn=0 */
            y_norm = atoff(s_val);
            read2();                            /* Tag=30, Value=Zn=0 */
            z_norm = atoff(s_val);
            flag = atoi(read2());               /* Flag vertex POLYMESH,Tag=70,Value=128 */

            if (flag != 128)
            {
                fprintf(stderr, "\nDXF: Invalid structure Vertix face - %d", i);
                exit(-3);
            }

            v1 = atoi(read2());  /* Tag=71, Value=A,*/
            v2 = atoi(read2());  /* Tag=72, Value=B */
            v3 = atoi(read2());  /* Tag=73, Value=C */
            nv[0] = (v1 < 0) ? (v1 * -1) - 1 : (v1 - 1);
            nv[1] = (v2 < 0) ? (v2 * -1) - 1 : (v2 - 1);
            nv[2] = (v3 < 0) ? (v3 * -1) - 1 : (v3 - 1);

            /*----- Counting number edges ----*/
            if (v1 < 0)  Nedge += 2;
            if (v2 < 0)  Nedge += 2;
            if (v3 < 0)  Nedge += 2;
            if (v1 > 0 && v2 > 0 && v3 > 0)  Nedge += 3;

            /* Writing first vertex-0 for face-0 to HRC-file */
            if (i == 0)    /* Vertex to face=4,(3), begin block */
            {
                vert_face1 = (int)VFACE;
                swap((char*)&vert_face1, sizeof(vert_face1));
                err = fwrite(&vert_face1, sizeof(vert_face1), 1, hrc);
                ferr(err, 1);
                face.vert_num = nv[0];
                swap((char*)&face.vert_num, sizeof(face.vert_num));
                face.x_normal = x_norm;
                swap((char*)&face.x_normal, sizeof(face.x_normal));
                face.y_normal = y_norm;
                swap((char*)&face.y_normal, sizeof(face.y_normal));
                face.z_normal = z_norm;
                swap((char*)&face.z_normal, sizeof(face.z_normal));
                err = fwrite(&face, sizeof(face), 1, hrc);
                ferr(err, 1);
                for (j = 1; j < VFACE; j++)
                {
                    face.vert_num = nv[j];
                    swap((char*)&face.vert_num, sizeof(face.vert_num));
                    face.x_normal = x_norm;
                    swap((char*)&face.x_normal, sizeof(face.x_normal));
                    face.y_normal = y_norm;
                    swap((char*)&face.y_normal, sizeof(face.y_normal));
                    face.z_normal = z_norm;
                    swap((char*)&face.z_normal, sizeof(face.z_normal));
                    err = fwrite(&face, sizeof(face), 1, hrc);
                    ferr(err, 1);
                }
            }

            /* Writing follow verticies for face-1 ... face-n */
            else {
                vert_face = (long)VFACE;
                swap((char*)&vert_face, sizeof(vert_face));
                err = fwrite(&vert_face, sizeof(vert_face), 1, hrc);
                ferr(err, 1);
                for (j = 0; j < VFACE; j++)
                {
                    face.vert_num = nv[j];
                    swap((char*)&face.vert_num, sizeof(face.vert_num));
                    face.x_normal = x_norm;
                    swap((char*)&face.x_normal, sizeof(face.x_normal));
                    face.y_normal = y_norm;
                    swap((char*)&face.y_normal, sizeof(face.y_normal));
                    face.z_normal = z_norm;
                    swap((char*)&face.z_normal, sizeof(face.z_normal));
                    err = fwrite(&face, sizeof(face), 1, hrc);
                    ferr(err, 1);
                }
            }

            if (flist)
            {
                printf("\n   Face[%d]= %d", i, nv[0]);
                printf("   Face[%d]= %d", i, nv[1]);
                printf("   Face[%d]= %d", i, nv[2]);
            }

        } /* end if-1 */
        else --i;

    } /* end for-1 */
}

/*--------------------- 
     conv_edges() 
----------------------*/
void conv_edges(void)
{
    int   i;
    short zero;
    long num_edge;
    struct s_edge_fore fore;
    struct s_edge_six  six;

    printf("\n  Converting edges...\n");

    /* Writing info about number EDGES */
    zero = 0;
    err = fwrite(&zero, sizeof(zero), 1, hrc);
    ferr(err, 1);
    num_edge = Nedge;
    swap((char*)&num_edge, sizeof(num_edge));
    err = fwrite(&num_edge, sizeof(num_edge), 1, hrc);
    ferr(err, 1);

    /* Seting current position for start vertex of faces to DXF */
    if (fsetpos(dxf, pos) != 0)
    {
        fprintf(stderr, "\nError writing DXF (pass 2)...");
        exit(1);
    }

    for (i = 0; i < Nface; i++)                 /* for -1 */
    {
        read2();                                /*   Tag=0, Val="VERTEX" */
        if (memcmp(s_val, "VERTEX", 6) == 0)    /* if-1 */
        {
            read2();                            /* Passing Tag=8, Value=Name_layer */
            read2();                            /* Tag=10, Value=Xn=0 */
            x_norm = atoff(s_val);
            read2();                            /* Tag=20, Value=Yn=0 */
            y_norm = atoff(s_val);
            read2();                            /* Tag=30, Value=Zn=0 */
            z_norm = atoff(s_val);
            flag = atoi(read2());               /* Flag vertex POLYMESH,Tag=70,Value=128 */

            if (flag != 128)
            {
                fprintf(stderr, "\nDXF: Invalid structure Vertix face - %d", i);
                exit(-3);
            }

            v1 = atoi(read2());  /* Tag=71, Value=A,*/
            v2 = atoi(read2());  /* Tag=72, Value=B */
            v3 = atoi(read2());  /* Tag=73, Value=C */
            nv[0] = (v1 < 0) ? (v1 * -1) - 1 : (v1 - 1);
            nv[1] = (v2 < 0) ? (v2 * -1) - 1 : (v2 - 1);
            nv[2] = (v3 < 0) ? (v3 * -1) - 1 : (v3 - 1);

            /*----- Writing vision edges -----*/
            if (v1 < 0)
            {
                fore.e1 = nv[1];
                swap((char*)&fore.e1, sizeof(fore.e1));
                fore.e2 = nv[2];
                swap((char*)&fore.e2, sizeof(fore.e2));
                fore.m1 = MEDGE;
                fore.e3 = nv[2];
                swap((char*)&fore.e3, sizeof(fore.e3));
                fore.e4 = nv[0];
                swap((char*)&fore.e4, sizeof(fore.e4));
                fore.m2 = MEDGE;

                err = fwrite(&fore, sizeof(fore), 1, hrc);
                ferr(err, 1);
                Nedge += 2;
            }

            if (v2 < 0)
            {
                fore.e1 = nv[0];
                swap((char*)&fore.e1, sizeof(fore.e1));
                fore.e2 = nv[1];
                swap((char*)&fore.e2, sizeof(fore.e2));
                fore.m1 = MEDGE;
                fore.e3 = nv[2];
                swap((char*)&fore.e3, sizeof(fore.e3));
                fore.e4 = nv[0];
                swap((char*)&fore.e4, sizeof(fore.e4));
                fore.m2 = MEDGE;

                err = fwrite(&fore, sizeof(fore), 1, hrc);
                ferr(err, 1);
                Nedge += 2;
            }

            if (v3 < 0)
            {
                fore.e1 = nv[0];
                swap((char*)&fore.e1, sizeof(fore.e1));
                fore.e2 = nv[1];
                swap((char*)&fore.e2, sizeof(fore.e2));
                fore.m1 = MEDGE;
                fore.e3 = nv[1];
                swap((char*)&fore.e3, sizeof(fore.e3));
                fore.e4 = nv[2];
                swap((char*)&fore.e4, sizeof(fore.e4));
                fore.m2 = MEDGE;

                err = fwrite(&fore, sizeof(fore), 1, hrc);
                ferr(err, 1);
                Nedge += 2;
            }

            /* For 3 vision edges */
            if (v1 > 0 && v2 > 0 && v3 > 0)
            {
                six.e1 = nv[0];
                swap((char*)&six.e1, sizeof(six.e1));
                six.e2 = nv[1];
                swap((char*)&six.e2, sizeof(six.e2));
                six.m1 = MEDGE;
                six.e3 = nv[1];
                swap((char*)&six.e3, sizeof(six.e3));
                six.e4 = nv[2];
                swap((char*)&six.e4, sizeof(six.e4));
                six.m2 = MEDGE;
                six.e5 = nv[2];
                swap((char*)&six.e5, sizeof(six.e5));
                six.e6 = nv[0];
                swap((char*)&six.e6, sizeof(six.e6));
                six.m3 = MEDGE;

                err = fwrite(&six, sizeof(six), 1, hrc);
                ferr(err, 1);
                Nedge += 3;
            }

        } /* end if-1 */
        else --i;
    } /* end for-1 */
}

/*------------------------ 
    (6) write_cen_axis 
-------------------------*/
void write_cen_axis(void)
{
    struct s_center center;

    center.zero = (short)ZERO;
    center.x0 = XAXIS;
    swap((char*)&center.x0, sizeof(center.x0));
    center.y0 = YAXIS;
    swap((char*)&center.y0, sizeof(center.y0));
    center.z0 = ZAXIS;
    swap((char*)&center.z0, sizeof(center.z0));
    err = fwrite(&center, sizeof(center), 1, hrc);
    ferr(err, 1);
}


/*-------------------- 
        blocks() 
---------------------*/
void blocks(void)
{
    fgets(s_tag, SIZE, dxf);     /* Tag = 0 */
    fgets(s_val, SIZE, dxf);     /* Value = "BLOCK" */
    if ((memcmp(s_val, "BLOCK", 5)) == 0)
        fprintf(stderr, "No executions blocks. Section \"BLOCKS\"...");

    /*..... Continue (no writing) for TOPAS and AutoCAD .....*/
}


/*------------------ 
        swap() 
-------------------*/
void swap(char* str, unsigned n)
{
    char b[4], * data;

    data = str;
    switch (n)
    {
    case 2:
        b[1] = *data++;
        b[0] = *data;
        *str++ = b[0];
        *str = b[1];
        break;
    case 4:
        b[3] = *data++;
        b[2] = *data++;
        b[1] = *data++;
        b[0] = *data;
        *str++ = b[0];
        *str++ = b[1];
        *str++ = b[2];
        *str = b[3];
        break;
    default:
        break;
    }
}

/*================= 
     atoff() 
==================*/
double atoff(char fstr[])
{
    double val, power;
    int i, sign;

    for (i = 0; isspace(fstr[i]); i++)
        ;
    sign = (fstr[i] == '-') ? -1 : 1;
    if (fstr[i] == '+' || fstr[i] == '-')
        i++;
    for (val = 0.0; isdigit(fstr[i]); i++)
        val = 10.0 * val + (fstr[i] - '0');
    if (fstr[i] == '.')
        i++;
    for (power = 1.0; isdigit(fstr[i]); i++)
    {
        val = 10.0 * val + (fstr[i] - '0');
        power *= 10.0;
    }
    return sign * val / power;
}

/*================= 
     strcopy 
==================*/
void strcopy(char* str1, char* str2)
{
    while ((*str1++ = *str2++) != '\0')
        ;
    *--str1 = '\0';
    *--str1 = '\0';
}

/*--------------------- 
         ferr() 
----------------------*/
void ferr(size_t ne, size_t nd)
{
    if (ne < nd)
    {
        fprintf(stderr, "\nERROR: Writing data error....");
        exit(-1);
    }
}

/*------------------------ 
        fexists() 
-------------------------*/
void fexists(char* fname)
{
    FILE* f;
    if ((f = fopen(fname, "r")) == NULL)
    {
        fclose(f); return;
    }
    else
    {
        fprintf(stderr, "\nThe file \"%s\" already exists....", fname);
        exit(-1);
    };
}

/*------------------- def_hrc_name() ----------------------*/
/************************************************************
void def_hrc_name(char *sname)
{
    char dev[4],dir[70],file[10],suff[5];
    char *ext = ".hrc";
    fnsplit(sname, dev, dir, file, suff);
    strcpy(file_hrc, strcat(file, ext));
}
**************************************************************/

/*----------------------- 
      control_dxf() 
------------------------*/
void control_dxf(void)
{
    int i;
    unsigned char c = 0;

    for (; isspace(c); c = getc(dxf));    /*  Passing spaces symbols */
    switch (c = getc(dxf))
    {
    case '0':
        if ((c = getc(dxf)) == '\n')
            for (i = 0; i < 9; i++)  cs[i] = c = getc(dxf);
        if (memcmp(cs, "SECTION", 7) == 0)
        {
            rewind(dxf);  return;
        }
        else
            break;
    case '9':
        cs[0] = c;
        for (i = 1; i <= 3; i++)  cs[i] = getc(dxf);
        if (memcmp(cs, "999", 3) == 0)
            if (cs[3] == '\n')
            {
                rewind(dxf); return;
            }
            else
                break;
    default: break;

    } /* end switch */

    fprintf(stderr, "\nDXF file corrupted or this is not DXF file...\n");
    exit(-1);
}

/*---------------------- 
       msg_title() 
-----------------------*/
void msg_title(void)
{
    fprintf(stderr, "\n(c) Copyright by S.Gasanov, September, 1994, v1.3");
    fprintf(stderr, "\nUkrainian, Kiev, Manufacture of Advertisement Co.,tel/fax:(044)295-94-87");
    fprintf(stderr, "\nTranslator from DXF file (3DS STUDIO) to HRC file (SOFTIMAGE): ");
    fprintf(stderr, "\nusage:     dxf2hrc.exe file_dxf [file_hrc] [options]");
    fprintf(stderr, "\noptions:  -h  --> This message");
    fprintf(stderr, "\n          -l  --> Output list Vertex & Faces\n");

    exit(1);
}
/*------------------------------ E N D ----------------------------------*/
