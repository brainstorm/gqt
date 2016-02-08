BIN=../bin
OBJ=../obj
HTS_PREFIX=$(shell pkg-config --variable=prefix htslib)
SQLITE_INCLUDES=$(shell pkg-config --variable=includedir sqlite3)
#CFLAGS=-g -D_FILE_OFFSET_BITS=64
CFLAGS=-O2 -D_FILE_OFFSET_BITS=64 -Werror -Wuninitialized
#CFLAGS=-Wall -O0 -g -fno-inline -fkeep-inline-functions -D_FILE_OFFSET_BITS=64 -fPIC -DDEBUG -D_DEBUG
#CFLAGS+=-mavx -mavx2
#CFLAGS+=-Dtime_wahbm_pca_by_name
#CFLAGS+=-Dtime_count_range_records_compressed_in_place_wahbm
#CFLAGS+=-Dtime_count_range_records_in_place_wahbm
#CFLAGS+=-Dtime_sum_range_records_in_place_wahbm
#CFLAGS+=-Dtime_convert_file_by_name_bcf_to_wahbm_bim
#CFLAGS+=-Dtime_sort_rotate_gt_md
CC=gcc
LEX=flex

LIB= timer.o \
		 misc.o \
		 query.o \
		 pop.o \
		 bim.o \
		 vid.o \
		 off.o \
		 parse_q.yy.o \
		 parse_q.o \
		 ped.o \
		 pq.o \
		 bcf.o \
		 quick_file.o \
		 output_buffer.o \
		 pthread_pool.o \
		 genotq.o \
		 convert.o \
		 plt.o \
		 wah.o \
		 wahbm.o \
		 wahbm_in_place.o \
		 wahbm_compressed_in_place.o \
		 ubin.o \
		 view.o \
		 sort.o \
		 count.o \
		 sandbox.o

.SUFFIXES:

OBJS=$(addprefix $(OBJ)/, $(LIB))

.SECONDARY: $(OBJS)


PROG=read_binary_uints \
	gqt 

LIST=parse_q $(addprefix $(BIN)/, $(PROG))

all: $(LIST)

parse_q: 
	$(LEX) -oparse_q.yy.c parse_q.l


$(OBJ)/%.o: %.c 
	$(CC) -c $(CFLAGS) -o $@ $< \
		-I$(HTS_PREFIX)


$(BIN)/%: %.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ \
		-DSAMTOOLS=1 \
		-I$(HTS_PREFIX) \
		-I$(SQLITE_INCLUDES) \
		-ldl -lz -lm -pthread -lsqlite3 -lhts
