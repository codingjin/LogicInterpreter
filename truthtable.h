#define SIZE 4096

char inputtoken[32];
FILE *fp;
void deal_input();
void deal_output();
void deal_gate();
void set_io_table();
void set_temp_table();

typedef struct Token {
	int value;
	int hashcode;
	char key[32];
}Token;
Token *input, *output, *temp;
int inputsize, outputsize, tempsize;
void printarr(Token *arr, int size);

typedef struct TNode {
	Token token;
	struct TNode *next;
}TNode;

typedef struct TList {
	TNode *head;
	TNode *last;
	int size;
}TList;
TList tlist;

typedef enum {INPUT=1, OUTPUT=2, TEMP=3} var_type;
typedef enum {AND=1, OR=2, NAND=3, NOR=4, XOR=5, NOT=6, PASS=7,
				DECODER=8, MULTIPLEXER=9} op_type;

typedef struct Gate {
	op_type otype;
	int size;
	int insize;
	int outsize;
	Token *in;
	Token *out;
	Token *select;
	Token io[3]; 
	//AND, OR, NAND, NOR, XOR: io[0,1] is input, and io[2] output
	//NOT, PASS: io[0]is input, and io[2] output
	//MULTIPLEXER use io[2] as output
}Gate;
Gate *gate;
int gatesize;
void freegate();

typedef struct Node {
	var_type vtype;
	int index;
	int hashcode;
	struct Node *next;
	char key[32];
}Node;
Node *Table[SIZE];
void clear_table();

int Exist(char *key); // if not exist return 0, otherwise return 1
int Search(char *key); // return value, if not exist return -1
int Hash(char *key);
int Insert(char *key, var_type vtype, int index);
int Update(char *key, int value);
int Match(char *key, var_type vtype);

typedef struct GNode {
	Gate gate;
	struct GNode *next;
}GNode;

typedef struct GList {
	GNode *head;
	GNode *last;
	int size;
}GList;
GList glist;

op_type get_optype(char *op);
int is_unaop(op_type otype);
int is_binop(op_type otype);
int is_decoder(op_type otype);
int is_multiplexer(op_type otype);

int is_const(char *key);
int is_ignore(char *key);

void printglist(GNode *head);
void printotype(op_type otype);

void copygate(Gate *gp, GNode *gnp);

void printgate(Gate*, int);
void clear_glist();
void set_tlist();
void add_tlist(char*);
void print_tlist();

void notemp_compute();
void notemp_gate_compute();

void compute();
void gate_compute();
void set_tarr();
//void set_reftable();

int binstrtoi(char *str);

typedef struct RefNode {
	int gate_index;
	struct RefNode *next;
}RefNode;

typedef struct RefList {
	RefNode *head;
}RefList;
RefList *reftable; 
// for each bucket, its elements are sorted from small to large in a linklist

void InsertReftable(int bucket, int gate_index);
int ExistReftable(int bucket, int gate_index);

typedef struct GateDep {
	int degree;
	int mark;
}GateDep;
GateDep *gatedep;
int *gatequeue;
void build_gatequeue();

