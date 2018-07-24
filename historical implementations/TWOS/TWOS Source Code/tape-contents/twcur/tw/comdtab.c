/*      Copyright (C) 1989, 1991, California Institute of Technology.
		U. S. Government Sponsorship under NASA Contract NAS7-918
		is acknowledged.        */

/*
 * $Log:	comdtab.c,v $
 * Revision 1.15  91/12/27  08:41:24  reiher
 * Added tester commands for throttling and ratio migration
 * 
 * Revision 1.14  91/11/06  11:08:46  configtw
 * Fix compile errors.
 * 
 * Revision 1.13  91/11/04  10:42:40  pls
 * Add ALLOWNOW and MAXMSGPOOL.
 * 
 * Revision 1.12  91/11/01  09:29:27  reiher
 * Added tester commands for throttling, critical path, and new dynamic load 
 * management capabilities (PLR)
 * 
 * Revision 1.11  91/07/22  15:43:53  configtw
 * Fix misspelling of MARK3.
 * 
 * Revision 1.10  91/07/22  14:33:12  configtw
 * Remove recv_q_limit stuff for Mark3.
 * 
 * Revision 1.9  91/07/17  15:07:27  judy
 * New copyright notice.
 * 
 * Revision 1.8  91/07/09  15:20:09  steve
 * Added set receive q limit, set object timing mode and changable notabug
 * 
 * Revision 1.7  91/06/03  12:23:45  configtw
 * Tab conversion.
 * 
 * Revision 1.6  91/05/31  12:49:20  pls
 * Add LISTHDR command.
 * 
 * Revision 1.5  91/04/01  15:34:51  reiher
 * Added a few commands to tester for dynamic load management.  They include
 * a graphics toggle, a phase create command, a command to dump ocbs by
 * address, and a command to show the contents of the type table.
 * 
 * Revision 1.4  91/03/28  09:59:18  configtw
 * Change timeoff() to tw_timeoff()--conflict with Sun libraries.
 * 
 * Revision 1.3  91/03/26  09:08:48  pls
 * 1.  Remove GVTPOS.
 * 2.  Add HOGLOG.
 * 3.  Add SCHEDULE as synonym for TELL.
 * 4.  Add TYPEINIT.
 * 5.  Change DELFILE so it doesn't broadcast.
 * 
 * Revision 1.2  90/11/27  09:29:24  csupport
 * add staddrt command (plr)
 * 
 * Revision 1.1  90/08/07  15:38:03  configtw
 * Initial revision
 * 
*/
char comdtab_id [] = "@(#)comdtab.c     1.53\t10/2/89\t14:12:10\tTIMEWARP";


#include "twcommon.h"
#include "twsys.h"
#include "tester.h"

#ifdef BF_PLUS  /* Unfortunate kludge because of illegal  */
#define BBN     /* redefinitions if we include 'machdep.h */
#endif          /* and stdio.h'.                          */
#ifdef BF_MACH
#define BBN
#endif



static Name     rcver;
static STime    rcvtim;
static int      sel;
static char     message[300];
static int      msgnum;
static Name     snder;
static STime    sndtim;
static Symbol   mtype;
static int		ttype;
static int      node;
static Name     obj_name;
static STime    stime;
static Name     objtype;
static Name     name;
static int      level;
static Hex      msgh;
static int      number;
static Hex      state;
static Hex      ocb;
static Hex      listElement;
static int      memsize;
static int      flow_log_size;
static int      msg_log_size;
static STime    hog_log_time;
static int      intrvl;
static STime    chg_time;
static int      penalty;
static int      reward;
static int      plimit;
static STime    window;
static int		events;
static double   windowMult;
static double	tmult;
static int		tadd;
static int 		maxLength;
static int      address;
static double   delay;
static int      IS_delta;
static int      IS_log_size;
static double   ptime;
static STime  begin;
static STime  end;
static int      threshold;
static int		mratio;
static int      numMigr = 1;
static int      splitIn;
static int      chooseIn;
static int      maxSimultMigr;
static int      cycles;
static int      migrationCost;
#ifdef BBN
static char     rf_path[80];
static char     rf_file[80];
#endif
static int		numbuffs;

#define I(x) (int *)(x)

ARG_DEF arg_defs [] =
{
	{ "RECEIVER",       NAME,           I(rcver)        },
	{ "RECEIVE TIME",   STIME,          I(&rcvtim)      },
	{ "SELECTOR",       INTEGER,        &sel            },
	{ "MESSAGE",        STRING,         I(message)      },
	{ "MESSAGE NUMBER", INTEGER,        &msgnum         },
	{ "SENDER",         NAME,           I(snder)        },
	{ "SEND TIME",      STIME,          I(&sndtim)      },
	{ "MESSAGE TYPE",   SYMBOL,         &mtype          },
	{ "NODE",           INTEGER,        &node           },
	{ "OBJECT NAME",    NAME,           I(obj_name)     },
	{ "SIMULATION TIME",STIME,          I(&stime)       },
	{ "TIME INTERVAL",  INTEGER,        &intrvl         },
	{ "CHANGE TIME",    STIME,          I(&chg_time)    },
	{ "OBJECT TYPE",    NAME,           I(objtype)      },
	{ "FUNCTION NAME",  NAME,           I(name)         },
	{ "MONITOR LEVEL",  INTEGER,        &level          },
	{ "MSGH POINTER",   HEX,            &msgh           },
	{ "COMMAND NUMBER", INTEGER,        &number         },
	{ "MAX ACKS",       INTEGER,        &number         },
	{ "STACK SIZE",     INTEGER,        &number         },
	{ "STATE POINTER",  HEX,            &state          },
	{ "LIST ELEMENT",   HEX,            &listElement    },
	{ "MEM KBYTES",     INTEGER,        &memsize        },
	{ "FLOW LOG SIZE",  INTEGER,        &flow_log_size  },
	{ "MSG LOG SIZE",   INTEGER,        &msg_log_size   },
	{ "HOG LOG TIME",   STIME,          I(&hog_log_time)},
	{ "PENALTY NUMBER", INTEGER,        &penalty        },
	{ "REWARD NUMBER",  INTEGER,        &reward         },
	{ "PEEK LIMIT",     INTEGER,        &plimit         },
	{ "TIME WINDOW",    STIME,          I(&window)      },
	{ "EVENT WINDOW",   INTEGER ,       &events      	},
	{ "WINDOW MULT",	REAL,			&windowMult		},
	{ "THROT MULT",		REAL,			&tmult			},
	{ "THROT ADD",  	INTEGER,		&tadd			},
	{ "THROT TYPE",		INTEGER,		&ttype			},
	{ "IQ LENGTH",		INTEGER,		&maxLength		},
	{ "ADDRESS",        HEX,            &address        },
	{ "DELAY",          REAL,           I(&delay)       },
	{ "IS DELTA",       INTEGER,        &IS_delta       },
	{ "IS LOG SIZE",    INTEGER,        &IS_log_size    },
	{ "PHASE TIME",     REAL,           I(&ptime)       },
	{ "THRESHOLD",      INTEGER,        &threshold      },
	{ "MIGRATIO",       INTEGER,        &mratio      	},
	{ "MIGRATIONS",     INTEGER,        &numMigr        },
	{ "SPLITSTRAT",     INTEGER,        &splitIn        },
	{ "CHOOSESTRAT",    INTEGER,        &chooseIn       },
	{ "MAXOFF",         INTEGER,        &maxSimultMigr  },
	{ "IDLEDLM",        INTEGER,        &cycles         },
	{ "DLMINT",         INTEGER,        &intrvl         },
	{ "PERMIGRCOST",	INTEGER,		&migrationCost 	},
#ifdef BBN
	{ "PATH",           STRING,         I(rf_path)      },
	{ "FILE",           STRING,         I(rf_file)      },
#endif
	{ "BEGIN",                STIME,          I(&begin)       },
	{ "END",          STIME,          I(&end)         },
	{ 0 }
};

int getfile ();
int putfile ();
int delfile ();
int typeinit ();
int help ();
int now_cmd ();
int myName_cmd ();
int obcreate_cmd ();
int phcreate_cmd ();
int tell_cmd ();
void allowNow ();
int numMsgs_cmd ();
int msg_cmd ();

#ifdef MONITOR
int monon ();
int set_level ();
int list_levels ();
int monobj ();
#endif
int monoff ();

int memanal ();
int manual_lvt ();
int manual_gvt ();
int gvt_position ();
int manual_objend ();
int clear_screen ();
int go ();
int timeon ();
int tw_timeoff ();
int timeval ();
int timechg ();

int dumpmsgx ();
int dumpstatex ();
int dumpstateAddrTablex ();
int dm ();
int dst ();
int showschedq ();
int showdeadq ();
int showocb ();
int showtypes ();
int showListHdr();

int dump_ocb_by_name ();
int show_iq_by_name ();
int show_oq_by_name ();
int show_sq_by_name ();
int showTruncSQ_by_name ();
int mem_used_in_queues ();

int show_miq_by_name ();
int show_moq_by_name ();
int show_msq_by_name ();

/* Added show_mocb_by_name();  PLRBUG */

int show_mocb_by_name ();

int dump_ocb_by_phase ();
int show_iq_by_phase ();
int show_oq_by_phase ();
int show_sq_by_phase ();

int ShowCacheEntry();
int ShowHomeListEntry();
int showHomeNode ();

int set_object_breakpoint ();
int set_time_breakpoint ();
int clear_breakpoint ();
int show_breakpoint ();

int set_object_watchpoint ();
int set_time_watchpoint ();
int clear_watchpoint ();
int show_watchpoint ();

int set_memsize ();
int stop ();

int set_nostdout ();
int set_nogvtout ();
int print_acks ();
int print_queues ();

#ifdef SUN
int dump_socket ();
int debug ();
#endif

int enable_mem_stats ();
int set_max_acks ();
int set_max_neg_acks ();
int	setMaxFreeMsgs();
int set_recv_q_limit ();
int set_its_a_feature ();
int set_obj_time_mode ();
int set_objstksize ();
int set_pktlen ();

int enable_file_echo ();
int disable_file_echo ();

int disable_message_sendback ();
int enable_aggressive_cancellation ();

int set_penalty ();
int set_reward ();

int set_time_window ();
int turnThrottleOn ();
int setEventThrottle ();
int setThrotMultFactor ();
int setThrotAddFactor ();
int setWindowMultiplier ();
int setMaxIQLen ();

int DumpHomeList ();
int DumpPendingList ();
int DumpCache ();

#ifdef MARK3

int set_plimit ();
int timetest ();
int watch ();
int debug ();

int disable_interrupts ();

#endif

int flowlog ();
int dumplog ();
int msglog ();
int dump_mlog ();
void hoglog();

int enableCritPath();

int set_prop_delay ();
int set_gvt_sync ();
int gvtinit ();
int subcube ();

#ifdef TRANSPUTER

int clockval ();
int test_xrecv ();
int test_xsend ();
int cdebug ();
int dump_kmsgh ();
int show_kq_ifc ();

#endif

int split_object_cmd ();
int move_phase_cmd ();
int PrintsendStateQ ();
int PrintsendOcbQ ();
int showStatesMovedQ	() ;

#ifdef EVTLOG

int set_evtlog ();
int set_chklog ();

#endif

int init_islog ();
int IS_dumplog ();

int set_cpulog ();
int set_cutoff_time ();

#ifdef DLM

int set_dlm ();
int set_threshold();
int setMigrRatio();
int setBatch();
int setNumMigrs();
int setSplitStrat ();
int setChooseStrat ();
int setMaxMigr ();
int setIdleDlmCycles ();
int setDlmInt ();
int setMigrGraph ();
int turnRatioMigrationOn ();
#ifdef BBN
int showsendbuffs ();
#endif
#endif  DLM


#ifdef BF_PLUS
int rfget_cmd ();
#endif

int quelog ();
int dump_qlog ();

FUNC_DEF func_defs [] =
{
	{ "QUELOG",         "MESSAGE QUEUE LOG",    BCAST,
						quelog,                 &number                 },
#ifdef DLM

	{ "DLM",            "ENABLE DYNAMIC LOAD MANAGEMENT",       BCAST,
						set_dlm                                         },

	{ "THRESH",         "SET DLM UTILIZATION THRESHOLD",        BCAST,
						set_threshold,          &threshold              },

	{ "RATIODLM",       "MIGRATE BY RATIO INSTEAD OF DIFFERENCE", BCAST,
						turnRatioMigrationOn,          0              	},

	{ "MRATIO",         "SET DLM UTILIZATION RATIO",        	BCAST,
						setMigrRatio,          &mratio              	},

	{ "BATCH", 			"RUN TWOS IN BATCH MODE",				BCAST,
						setBatch 										},

	{ "MIGRATIONS",     "SET # OF MIGRATIONS PER INTERVAL",     BCAST,
						setNumMigrs,            &numMigr                },

	{ "SPLITSTRAT",     "METHOD OF SPLITTING PHASE FOR DLM",    BCAST,
						setSplitStrat,          &splitIn                },

	{ "CHOOSESTRAT",    "METHOD OF CHOOSING PHASE FOR DLM",     BCAST,
						setChooseStrat,         &chooseIn               },

	{ "MAXOFF",         "MAX SIMULTANEOUS MIGRATIONS PER NODE", BCAST,
						setMaxMigr,             &maxSimultMigr          },

	{ "IDLEDLM",        "NUMBER OF INITIAL CYCLES WITHOUT DLM", BCAST,
						setIdleDlmCycles,               &cycles         },

	{ "DLMINT",         "INTERVAL BETWEEN DLM CYCLES",          BCAST,
						setDlmInt,                      &intrvl         },

	{ "DLMGRAPH",     "TOGGLE DLM GRAPHICS OUTPUT",           BCAST,
					  setMigrGraph                                      },

#endif DLM


	{ "DUMPQLOG",       "DUMP QUEUE LOG",       0,
						dump_qlog                                       },

	{ "CPULOG",         "WRITE CPU LOG",        0,
						set_cpulog                                      },

	{ "CUTOFF",         "SET CUTOFF TIME",      BCAST,
						set_cutoff_time,        I(&stime),              },

#ifdef EVTLOG

	{ "EVTLOG",         "WRITE EVENT LOG",      BCAST,
						set_evtlog                                      },

	{ "CHKLOG",         "CHECK EVENT LOG",      BCAST,
						set_chklog                                      },
#endif

#ifdef DLM

	{ "SPLIT",          "SPLIT OBJECT",         0,
						split_object_cmd,       I(name),I(&stime)       },

	{ "MOVE",           "MOVE PHASE",           0,
						move_phase_cmd, I(name),I(&stime),&node         },

	{ "SENDSTATEQ",     "PRINT QUEUE OF UNMIGRATED STATES",     0,
						PrintsendStateQ                                 },

	{ "SENDOCBQ",       "PRINT QUEUE OF UNMIGRATED OCBS",       0,
						PrintsendOcbQ                                   },

	{ "STATESRECVQ",	"SHOW QUEUE OF STATES WAITING ACKS",	0,
						showStatesMovedQ								},
#endif DLM

	{ "SUBCUBE",        "RUN ON SUBCUBE",       BCAST,
						subcube,        &node,  &number,  I(message)    },

	{ "GVTSYNC",        "SET GVT SYNC",         BCAST,
						set_gvt_sync,           &number                 },

	{ "GVTINIT",        "START GVT TIMER",      0,
						gvtinit,                                        },

	{ "WINDOW",         "SET TIME WINDOW",      BCAST,
						set_time_window,        I(&window)              },

	{ "THROTTLE",		"TURN THROTTLING ON",  BCAST,
						turnThrottleOn,			&ttype					},

	{ "ETHROTTLE",      "SET EVENT THROTTLE PARAMETER",      BCAST,
						setEventThrottle,        &events              	},

	{ "TMULT",      	"SET THROTTLE MULT PARAMETER",      BCAST,
						setThrotMultFactor,        I(&tmult)          	},
	
	{ "TADD",      		"SET THROTTLE ADD PARAMETER",      BCAST,
						setThrotAddFactor,        &tadd              	},

	{ "WSIZE",      	"SET THROTTLING WINDOW SIZE",      	BCAST,
						setWindowMultiplier,        I(&windowMult)     	},

	{ "MAXIQLEN",      	"LIMIT LENGTH OF INPUT QUEUE",      	BCAST,
						setMaxIQLen,        &maxLength             		},

	{ "GETFILE",        "GET FILE INTO MEMORY", BCAST,
						getfile,        I(message), I(name)             },

	{ "PUTFILE",        "CREATE OUTPUT FILE",   BCAST,
						putfile,        I(message), I(name), &node      },

	{ "DELFILE",        "DELETE INPUT FILE",    0,
						delfile,        I(name)                         },

	{ "TYPEINIT",       "INITIALIZE A TYPE",    BCAST,
						typeinit,       I(objtype),	I(name)				},

	{ "HELP",           "DISPLAY COMMANDS",     0,
						help                                            },

	{ "ACKS",           "PRINT ACKS PENDING",   0,
						print_acks                                      },

	{ "QUEUES",         "PRINT PENDING MESSAGE QUEUES", 0,
						print_queues                                    },

#ifdef SUN

	{ "SOCKET",         "DUMP SOCKET",          0,
						dump_socket,    &node                           },

	{ "DEBUG",          "CALL THE DEBUGGER",    0,
						debug                                           },

#endif

	{ "NOW",            "GET SIMULATION TIME",  0,
						now_cmd                                         },

	{ "MYNAME",         "GET OBJECT NAME",      0,
						myName_cmd                                      },

	{ "OBCREATE",       "CREATE OBJECT",        0,
						obcreate_cmd,   I(rcver), I(objtype), &node     },

	{ "PHCREATE",     "CREATE PHASE", 0,
					  phcreate_cmd,   I(rcver), I(objtype), &node,
					  I(&begin), I(&end)                                },

	{ "TELL",           "SEND EVENT MESSAGE",   0,
						tell_cmd,       I(rcver), I(&rcvtim), &sel, I(message)},

	{ "SCHEDULE",       "SEND EVENT MESSAGE",   0,
						tell_cmd,       I(rcver), I(&rcvtim), &sel, I(message)},

	{ "ALLOWNOW",		"ALLOW MESSAGES WITH TIME NOW",	BCAST,
						allowNow										},

	{ "NUMMSGS",        "GET MESSAGE COUNT",    0,
						numMsgs_cmd                                     },

	{ "MSG",            "GET MESSAGE TEXT",     0,
						msg_cmd,        &msgnum                         },

	{ "OBJEND",         "OBJECT END",           0,
						manual_objend                                   },
#ifdef MONITOR

	{ "MONON",          "MONITOR ON",           BCAST,
						monon                                           },

	{ "MLEVEL",         "MONITOR LEVEL",        0,
						set_level,      I(name), &level                 },

	{ "MLIST",          "MONITOR LIST LEVELS",  0,
						list_levels                                     },

	{ "MONOBJ",         "MONITOR ONE OBJECT",   BCAST,
						monobj,         I(obj_name)                     },

	{ "MONOFF",         "MONITOR OFF",          BCAST,
						monoff                                          },
#endif

	{ "MEMANAL",        "MEMORY ANALYSIS",      0,
						memanal                                         },

	{ "LVT",            "LOCAL VIRTUAL TIME",   0,
						manual_lvt                                      },

	{ "GVT",            "GLOBAL VIRTUAL TIME",  0,
						manual_gvt                                      },

	{ "CLR",            "CLEAR SCREEN",         0,
						clear_screen                                    },

	{ "GO",             "GO",                   0,
						go                                              },

	{ "TIMEON",         "INTERVAL TIMER ON",    0,
						timeon                                          },

	{ "TIMEOFF",        "INTERVAL TIMER OFF",   0,
						tw_timeoff                                      },

	{ "TIMEVAL",        "INTERVAL TIMER VALUE", 0,
						timeval,                &intrvl                 },

	{ "TIMECHG",        "INTERVAL CHANGE TIME", 0,
						timechg,                I(&chg_time)            },

	{ "SHOWSCHEDQ",     "SHOW SCHED QUEUE",     0,
						showschedq                                      },

	{ "SHOWDEADQ",     "SHOW DEAD OCB QUEUE",   0,
						showdeadq                                      },

	{ "DUMPMSG",        "DUMP MESSAGE",         0,
						dumpmsgx,               &msgh                   },

	{ "DUMPSTATE",      "DUMP STATE",           0,
						dumpstatex,             &state                  },

	{ "STADDRT",        "DUMP STATE ADDR TABLE",                0,
						dumpstateAddrTablex,    &state                  },

	{ "DM",             "DISPLAY MESSAGE",      0,
						dm,                     &msgh                   },

	{ "DST",            "DISPLAY STATE",        0,
						dst,                    &state                  },

	{ "DOCB",           "DUMP OCB BY NAME",     0,
						dump_ocb_by_name,       I(name)                 },
	 { "ADOCB",                "DUMP OCB BY ADDR",     0,
					  showocb,                &ocb                      },

	{ "SHOWTYPES",    "SHOW CONTENTS OF TYPE TABLE",  0,
					  showtypes                                         },

	{ "LISTHDR",        "SHOW LIST HEADER",     0,
					  showListHdr,              &listElement            },

	{ "IQ",             "SHOW INPUT QUEUE",     0,
						show_iq_by_name,        I(name)                 },

	{ "OQ",             "SHOW OUTPUT QUEUE",    0,
						show_oq_by_name,        I(name)                 },

	{ "SQ",             "SHOW STATE QUEUE",     0,
						show_sq_by_name,        I(name)                 },

	{ "MIQ",            "SHOW MIGRATING INPUT QUEUE",   0,
						show_miq_by_name,       I(name)                 },

	{ "MOQ",            "SHOW MIGRATING OUTPUT QUEUE",  0,
						show_moq_by_name,       I(name)                 },

	{ "MSQ",            "SHOW MIGRATING STATE QUEUE",   0,
						show_msq_by_name,       I(name)                 },

	{ "MOCB",           "SHOW MIGRATING OCB QUEUE",     0,
						show_mocb_by_name,      I(name)                 },

	{ "PDOCB",          "DUMP OCB BY PHASE",    0,
						dump_ocb_by_phase,      I(name), I(&ptime)      },

	{ "PIQ",            "SHOW INPUT QUEUE BY PHASE",    0,
						show_iq_by_phase,       I(name), I(&ptime)      },

	{ "POQ",            "SHOW OUTPUT QUEUE BY PHASE",   0,
						show_oq_by_phase,       I(name), I(&ptime)      },

	{ "PSQ",            "SHOW STATE QUEUE BY PHASE",    0,
						show_sq_by_phase,       I(name), I(&ptime)      },

	{ "TMEM",           "TOTAL MEMORY USED IN QUEUES",  0,
						mem_used_in_queues                              },

	{ "NOSTDOUT",       "DISABLE STDOUT",       BCAST,
						set_nostdout                                    },

	{ "NOGVTOUT",       "DISABLE GVT OUTPUT",   0,
						set_nogvtout                                    },

	{ "MEMSTATS",       "ENABLE MEMORY STATISTICS",     BCAST,
						enable_mem_stats                                },

	{ "MAXACKS",        "SET MAX ACKS",         BCAST,
						set_max_acks,           &number                 },

#ifndef BBN
	{ "MAXNEGACKS",     "SET MAX ACKS FOR ANTIMESSAGES",        BCAST,
						set_max_neg_acks,       &number                 },
#endif

	{ "MAXMSGPOOL",		"SET LIMIT FOR msg_free_pool",        BCAST,
						setMaxFreeMsgs,			&number                 },

#if !MARK3
	{ "RECVQLIMIT",     "SET RECEIVE QUEUE LIMIT",        BCAST,
						set_recv_q_limit,       &number                 },
#endif

	{ "OBJTIMEMODE",    "SET OBJECT TIMING MODE",        BCAST,
						set_obj_time_mode,       &number                 },

	{ "NOTABUG",     	"SET ITS A FEATURE",        BCAST,
						set_its_a_feature,       &number                 },

	{ "AGGRESSIVE",     "ENABLE AGGRESSIVE CANCELLATION",       BCAST,
						enable_aggressive_cancellation                  },

	{ "OBJSTKSIZE",     "SET OBJECT STACK SIZE",        BCAST,
						set_objstksize,         &number                 },

	{ "PKTLEN",         "SET PACKET LENGTH",    BCAST,
						set_pktlen,             &number                 },

	{ "FILEECHO",       "ENABLE FILE ECHO",     0,
						enable_file_echo                                },

	{ "NOFILEECHO",     "DISABLE FILE ECHO",    0,
						disable_file_echo                               },

	{ "BPO",            "BREAKPOINT OBJECT",    BCAST,
						set_object_breakpoint,  I(obj_name)             },

	{ "BPT",            "BREAKPOINT TIME",      BCAST,
						set_time_breakpoint,    I(&stime)               },

	{ "CBP",            "CLEAR BREAKPOINT",     BCAST,
						clear_breakpoint                                },

	{ "BP",             "SHOW BREAKPOINT",      0,
						show_breakpoint                                 },

	{ "WPO",            "WATCHPOINT OBJECT",    BCAST,
						set_object_watchpoint,  I(obj_name)             },

	{ "WPT",            "WATCHPOINT TIME",      BCAST,
						set_time_watchpoint,    I(&stime)               },

	{ "CWP",            "CLEAR WATCHPOINT",     BCAST,
						clear_watchpoint                                },

	{ "WP",             "SHOW WATCHPOINT",      0,
						show_watchpoint                                 },

	{ "MEMSIZE",        "SET MEMORY SIZE",      BCAST,
						set_memsize,            &memsize                },

	{ "NOSENDBACK",     "DISABLE MESSAGE SENDBACK",     BCAST,
						disable_message_sendback                        },

	{ "PENALTY",        "CANCELLATION PENALTY", BCAST,
						set_penalty,            &penalty                },

	{ "REWARD",         "CANCELLATION REWARD",  BCAST,
						set_reward,             &reward                 },

	{ "HOMELIST",       "SHOW HOME LIST",       0,
						DumpHomeList                                    },

	{ "PENDING",        "SHOW HL PENDING LIST", 0,
						DumpPendingList                                 },

	{ "CACHE",          "DUMP LOCATION CACHE",  0,
						DumpCache                                       },

	{ "CENTRY",         "SHOW LOCATION CACHE ENTRY",    0,
						ShowCacheEntry,         I(obj_name)             },

	{ "HENTRY",         "SHOW HOME LIST ENTRY", 0,
						ShowHomeListEntry,      I(obj_name)             },

	{ "HOME",           "SHOW HOME NODE",       0,
						showHomeNode,           I(obj_name)             },

	{ "STOP",           "STOP",                 0,
						stop                                            },

	{ "QUIT",           "QUIT",                 0,
						stop                                            },
#ifdef TRANSPUTER

	{ "CLOCKVAL",       "CLOCKVAL",     0,
						clockval                                        },

	{ "XRECV",          "XRECV",        0,
						test_xrecv                                      },

	{ "XSEND",          "XSEND",        0,
						test_xsend                                      },

	{ "CDEBUG",         "CDEBUG",       0,
						cdebug,                                         },

	{ "DUMPKMSGH",      "DUMPKMSGH",    0,
						dump_kmsgh,             & msgh                  },

	{ "SHOWKQ",         "SHOWKQ",       0,
						show_kq_ifc,            I(name)                         },

#endif

#ifdef MARK3

	{ "PLIMIT",         "MERCURY Q PEEK LIMIT", BCAST,
						set_plimit,             &plimit                 },

	{ "WATCH",          "DEBUG MERCURY",        0,
						watch,          &address                        },

	{ "TIMETEST",       "MARK3 TIMER TEST",     0,
						timetest                                        },

	{ "DEBUG",          "CALL THE DEBUGGER",    0,
						debug                                           },

	{ "NOINTS",         "DISABLE OBJECT INTERRUPTS",    BCAST,
						disable_interrupts                              },
#endif

	{ "FLOWLOG",        "ALLOCATE SPACE FOR FLOW LOG",  BCAST,
						flowlog,        &flow_log_size                  },

	{ "MSGLOG",         "ALLOCATE SPACE FOR MSG LOG",   BCAST,
						msglog,         &msg_log_size                   },

	{ "DUMPLOG",        "DUMP FLOW LOG",        0,
						dumplog                                         },

	{ "DUMPMLOG",       "DUMP MSG LOG",         0,
						dump_mlog                                       },

	{ "PROPDELAY",      "PROPORTIONAL DELAY FOR OBJECTS",       BCAST,
						set_prop_delay, I(&delay)                       },

	{ "ISLOG",          "INSTANTANEOUS SPEEDUP LOG",    BCAST,
						init_islog,     &IS_log_size, &IS_delta         },

	{ "IS_DUMPLOG",     "DUMP INSTANTANEOUS SPEEDUP LOG", BCAST,
						IS_dumplog                                      },

	{ "HOGLOG",         "Log longest uninterrupted objects",    BCAST,
						hoglog, I(&hog_log_time)						},


	{ "CRITPATH",		"Calculate the critical path",			BCAST,
						enableCritPath									},

	{ "TSQ",			"Show truncated state queue",			0,
						showTruncSQ_by_name,				I(name)		},


#ifdef BBN
	{ "SENDBUFFS",      "SHOW SEND BUFFERS",    0,
						showsendbuffs                                   },
#ifdef BF_PLUS

	{ "RFGET",          "RFGET COMMAND",        0,
						rfget_cmd,      I(rf_path), I(rf_file)          },
#endif
#endif

	{ 0 }
};


SYM_DEF sym_defs [] =
{
	{ "CMSG",           CMSG    },
	{ "EMSG",           EMSG    },
	{ "GVTSYS",         GVTSYS  },
	{ 0 }
};

help ()
{
	int i;

	_pprintf ( "\n" );

	for ( i = 0; func_defs[i].func_name != 0; i++ )
	{
	   _pprintf ( "%-30s%s\n", func_defs[i].func_name, func_defs[i].func_desc );
	}
}

