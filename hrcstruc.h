/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
              STRUCTURES DATA FOR HRC-FILE
                        HRCSTRUC.H
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/*
 * Date create:        05.09.94
 * Last modification:  18.09.94
 */
/*++++++++++++++ (1) Structure for NULL object ++++++++++++++++*/
struct s_infonull {
       short   code;
       short   mode;
       float   xnull;
       float   ynull;
       float   znull;
     };

/*+++++++++ (2) Structure for INFO MESH ++++++++++*/
struct s_info {
       short   type;        /* Type object - PMESH (byte = 04) */
       short   mod_smooth;  /* Automatic Discontinuity */
       float   val_smooth;  /* Smoothing faces of mesh */
       long    vertices;    /* Number vertex of mesh */
     };

/*+++++++++ (3) Structure for one VERTEX of mesh +++++++++*/
struct s_vertex {
       float   x_coord;   /* Coordinate X vertex of mesh */
       float   y_coord;   /* Coordinate Y vertex of mesh */
       float   z_coord;   /* Coordinate Z vertex of mesh */
       short   zero;      /* 2 bytes = 0 */
     };

/*++++++ (4) Structures for one FACE and NORMAL of vertex +++++*/
struct s_face {
       long    vert_num;     /* Number vertex to mesh(0,1,.) */
       float   x_normal;     /* Coordinate X vertex of normal */
       float   y_normal;     /* Coordinate Y vertex of normal */
       float   z_normal;     /* Coordinate Z vertex of normal */
     };

/*++++++ (5) Structure for one EDGE of mesh ++++++*/
struct s_edge_fore {
       long    e1;           /* Number start vertex */
       long    e2;           /* Number end vertex */
       short   m1;           /* 80 10 (80 00) */
       long    e3;           /* Number start vertex */
       long    e4;           /* Number end vertex */
       short   m2;           /* 80 10 (80 00) */
     };

struct s_edge_six {
       long    e1;           /* Number start vertex */
       long    e2;           /* Number end vertex */
       short   m1;           /* 80 10 (80 00) */
       long    e3;           /* Number start vertex */
       long    e4;           /* Number end vertex */
       short   m2;           /* 80 10 (80 00) */
       long    e5;           /* Number start vertex */
       long    e6;           /* Number end vertex */
       short   m3;           /* 80 10 (80 00) */
     };

/*++++++ (6) Structure for CENTER AXIS ++++++*/
struct s_center {
       short   zero;       /* 2 bytes = 0 */
       float   x0;         /*  X AXIS */
       float   y0;         /*  Y AXIS */
       float   z0;         /*  Z AXIS */
     };

/*++++++ (7) Structure for END DATA mesh ++++++*/
struct s_endmesh {
       long    zero1;
       long    mode;
       long    zero3;
       float   x_obj_cen;
       float   y_obj_cen;
       float   z_obj_cen;
       long    zero7;
       long    zero8;
       long    zero9;
       long    zero10;
       long    zero11;
     };

/*+++++++++++++++++++++++ End Structures ++++++++++++++++++++++*/
