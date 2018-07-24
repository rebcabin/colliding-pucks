
typedef union 
{
    int     int_val; /* Integer valued tokens. */
    long   long_val; /* Long    valued tokens. */
    double  dbl_val; /* Double  valued tokens. */
    char *  str_val; /* String  valued tokens. */ 
} YxYSTYPE;
extern YxYSTYPE yXylval;
# define NUL_TKN 257
# define NEST_TKN 258
# define OBCR_TKN 259
# define TIMEV_TKN 260
# define TIMECHG_TKN 261
# define MSG_TKN 262
# define RMINT_TKN 263
# define GO_TKN 264
# define NOSTDT_TKN 265
# define MACKS_TKN 266
# define END_TKN 267
# define MONON_TKN 268
# define MONOFF_TKN 269
# define OBJSTKSIZE_TKN 270
# define CUTOFF_TKN 271
# define DISP_TKN 272
# define GETF_TKN 273
# define PUTF_TKN 274
# define INT_TKN 275
# define LONG_TKN 276
# define DOUBLE_TKN 277
# define STR_TKN 278
# define PKTLEN_TKN 279
# define ISLOG_TKN 280
# define RFGET_TKN 281
# define DLM_TKN 282
# define IDLEDLM_TKN 283
# define MAXOFF_TKN 284
# define MIGRATIONS_TKN 285
# define DLMINT_TKN 286
# define OBSTATS_TKN 287
# define DELFILE_TKN 288
# define TPINIT_TKN 289
# define MAXPOOL_TKN 290
# define ALLOWNOW_TKN 291
