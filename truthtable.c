#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "truthtable.h"

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("invalid input\n");
		return 0;
	}
	fp = fopen(argv[1], "r");
	if (!fp) {
		printf("failed to open file[%s]\n", argv[1]);
		return 0;
	}
	deal_input();
	deal_output();
	set_io_table();
	deal_gate();
	set_tlist();
	if (!tlist.size)
		notemp_compute(); // all the clean work also done in the end
	else {
		set_tarr(); // initialize temp, and its index table;
					//tlist is clean inside
		build_gatequeue(); // reftable and gatedep are already clean inside
		compute(); // all the clean work also done in the end
	}

	return 0;
}

void printarr(Token *arr, int size)
{
	if (!arr || size<=0)	return;
	for (int i=0;i<size;++i)
		printf("index=%d key=%s value=%d\n", i, arr[i].key, arr[i].value);

}

int Exist(char *key)
{
	if (!key || strlen(key)==0)	return 0;
	
	int hashcode = Hash(key);
	int bucket = hashcode % SIZE; //Bucket(key);
	if (bucket<0)	bucket = -bucket;
	Node *current = Table[bucket];
	while (current) {
		if (current->hashcode==hashcode && strcmp(key, current->key)==0)
			return 1;

		current = current->next;
	}
	return 0;
}

int Search(char *key)
{
	if (!key || strlen(key)==0
		|| is_const(key)
		|| is_ignore(key))	return -1;

	int hashcode = Hash(key);
	int bucket = hashcode % SIZE;
	if (bucket<0)	bucket = -bucket;
	Node *current = Table[bucket];
	while (current) {
		if (current->hashcode==hashcode && strcmp(key, current->key)==0) {
			var_type vtype = current->vtype;
			int index = current->index;
			if (vtype == INPUT)	return input[index].value;	
			else if (vtype == OUTPUT)	return output[index].value;
			else if (vtype == TEMP)	return temp[index].value;
			else {
				printf("invalid vtype, lineno=%d\n", vtype);
				exit(0);
			}
		}
		current = current->next;
	}
	return -1;
}

int Hash(char *key)
{
	if (!key || strlen(key)==0)	return 0;

	int hashcode = 0;
	int len = strlen(key);
	if (len<=0)	return 0;

	for (int i=0;i<len;++i)	hashcode = hashcode + key[i];

	return hashcode;
}

int Insert(char *key, var_type vtype, int index)
{
	if (vtype!=INPUT && vtype!=OUTPUT && vtype!=TEMP) {
		printf("invalid type, insert failed\n");
		return 0;
	}
	if (Exist(key)) {
		printf("already exists, insert failed\n");
		return 0;
	}
	if (index<0) {
		printf("invalid index[%d]\n", index);
		return 0;
	}

	int hashcode = Hash(key);
	int bucket = hashcode % SIZE; // Bucket(key);
	if (bucket<0)	bucket = -bucket;

	Node *newnode = (Node*)malloc(sizeof(Node));
	if (!newnode) {
		printf("failed to malloc for newnode, lineno=%d\n", __LINE__);
		return 0;
	}
	newnode->vtype = vtype;
	newnode->index = index;
	newnode->hashcode = hashcode;
	newnode->next = NULL;
	strcpy(newnode->key, key);

	Node *current = Table[bucket];
	if (!current) // current==NULL
		Table[bucket] = newnode;
	else {
		Node *follow = current->next;
		current->next = newnode;
		newnode->next = follow;
	}
	return 1;

}

int Update(char *key, int value)
{
	if (!key || strlen(key)==0 || value<0 || value>1 || !Exist(key)) {
		printf("update failed! lineno=%d\n", __LINE__);
		return 0;
	}
	int hashcode = Hash(key);
	int bucket = hashcode % SIZE; // Bucket(key);
	if (bucket<0)	bucket = -bucket;
	Node *current = Table[bucket];
	while (current) {
		if (current->hashcode==hashcode && strcmp(current->key, key)==0) {
			//printf("vtype[%d]\n", current->vtype);
			if (current->vtype == INPUT) {
				if (!input || current->index>=inputsize || current->index<0) {
					printf("invalid node exists, lineno=%d\n", __LINE__);
					return 0;
				}
				input[current->index].value = value;
			}else if (current->vtype == OUTPUT) {
				if (!output || current->index>=outputsize || current->index<0) {
					printf("invalid node exists, linenoe=%d\n", __LINE__);
					return 0;
				}
				output[current->index].value = value;
			}else if (current->vtype == TEMP) {
				if (!temp || current->index>=tempsize || current->index<0) {
					printf("invalid node exists, lineno=%d\n", __LINE__);
					return 0;
				}
				temp[current->index].value = value;
				//printf("temp[%d], varname[%s], value[%d]\n", current->index
			}else {
				printf("invalid node with invalid vtype!\n");
				return 0;
			}
			return 1;
		}
		current = current->next;
	}
	return 0;

}

int Match(char *key, var_type vtype)
{
	if (!key || strlen(key)==0 
		|| is_const(key) 
		|| is_ignore(key))
		return -1;

	int hashcode = Hash(key);
	int bucket = hashcode % SIZE;
	if (bucket<0)	bucket=-bucket;
	Node *current = Table[bucket];
	while (current) {
		if (hashcode==current->hashcode 
			&& vtype==current->vtype
			&& strcmp(key, current->key)==0)
			return 1;

		current = current->next;
	}
	return -1;
}

op_type get_optype(char *op)
{
	if (!op || strlen(op)==0) {
		printf("invalid op type\n");
		exit(0);
	}else if (strcmp(op, "AND")==0)	return AND;
	else if (strcmp(op, "OR")==0)	return OR;
	else if (strcmp(op, "NAND")==0)	return NAND;
	else if (strcmp(op, "NOR")==0)	return NOR;
	else if (strcmp(op, "XOR")==0)	return XOR;
	else if (strcmp(op, "NOT")==0)	return NOT;
	else if (strcmp(op, "PASS")==0)	return PASS;
	else if (strcmp(op, "DECODER")==0)	return DECODER;
	else if (strcmp(op, "MULTIPLEXER")==0)	return MULTIPLEXER;
	else {
		printf("invalid op type[%s]\n", op);
		exit(0);
	}
}

int is_unaop(op_type otype)
{
	return otype==NOT || otype==PASS;
}

int is_binop(op_type otype)
{
	return otype==AND || otype==OR || otype==NAND 
		|| otype==NOR || otype==XOR;
}

int is_decoder(op_type otype)
{
	return otype==DECODER;
}

int is_multiplexer(op_type otype)
{
	return otype==MULTIPLEXER;
}

int is_const(char *key)
{ //strcmp(key, "0")==0 || strcmp(key, "1")==0;
	return key[0]=='0' || key[0]=='1';
}

int is_ignore(char *key)
{ // strcmp(key, "_")==0;
	return key[0]=='_';
}

void printglist(GNode *head)
{
	if (!head)	return;

	GNode *current = head;
	op_type otype;
	while (current) {
		otype = current->gate.otype;
		printotype(otype);
		if (is_unaop(otype))
			printf("%s %s\n", current->gate.io[0].key,
					current->gate.io[2].key);
		else if (is_binop(otype))
			printf("%s %s %s\n", current->gate.io[0].key,
					current->gate.io[1].key, current->gate.io[2].key);
		else if (is_decoder(otype)) {
			printf("size=%d insize=%d outsize=%d\n", current->gate.size,
					current->gate.insize, current->gate.outsize);
			printf("in of this DECODER: ");
			for (int i=0;i<current->gate.insize;++i)
				printf("%s ", current->gate.in[i].key);
			printf("\nout of this DECODER: ");
			for (int i=0;i<current->gate.outsize;++i)
				printf("%s ", current->gate.out[i].key);
			printf("\n");
		}else if (is_multiplexer(otype)) {
			printf("size=%d insize=%d\n", current->gate.size,
						current->gate.insize);
			printf("in of this MULTIPLEXER: ");
			for (int i=0;i<current->gate.insize;++i)
				printf("%s ", current->gate.in[i].key);
			printf("\nselect of this MULTIPLEXER: ");
			for (int i=0;i<current->gate.size;++i)
				printf("%s ", current->gate.select[i].key);
			printf("\nout of this MULTIPLEXER: %s\n", current->gate.io[2].key);
		}

		current = current->next;
	}

}

void printotype(op_type otype)
{
	if (otype==AND)	printf("AND ");
	else if (otype==OR)	printf("OR ");
	else if (otype==NAND)	printf("NAND ");
	else if (otype==NOR)	printf("NOR ");
	else if (otype==XOR)	printf("XOR ");
	else if (otype==NOT)	printf("NOT ");
	else if (otype==PASS)	printf("PASS ");
	else if (otype==DECODER)	printf("DECODER ");
	else if (otype==MULTIPLEXER)	printf("MULTIPLEXER ");
}

void copygate(Gate *gp, GNode *gnp)
{
	if (!gp || !gnp)	return;
	
	gp->otype = gnp->gate.otype;
	if (is_unaop(gp->otype)) {
		for (int i=0;i<3;i+=2) {
			gp->io[i].value = gnp->gate.io[i].value;
			strcpy(gp->io[i].key, gnp->gate.io[i].key);
			gp->io[i].hashcode = gnp->gate.io[i].hashcode;
		}
	}else if (is_binop(gp->otype)) {
		for (int i=0;i<3;++i) {
			gp->io[i].value = gnp->gate.io[i].value;
			strcpy(gp->io[i].key, gnp->gate.io[i].key);
			gp->io[i].hashcode = gnp->gate.io[i].hashcode;
		}
	}else if (is_decoder(gp->otype)) {
		gp->size = gnp->gate.size;
		gp->insize = gnp->gate.size;
		gp->outsize = gnp->gate.outsize;
		gp->in = gnp->gate.in;
		gp->out = gnp->gate.out;
	}else if (is_multiplexer(gp->otype)) {
		gp->size = gnp->gate.size;
		gp->insize = gnp->gate.insize;
		gp->in = gnp->gate.in;
		gp->select = gnp->gate.select;
		gp->io[2].value = -1;
		strcpy(gp->io[2].key, gnp->gate.io[2].key);
		gp->io[2].hashcode = gnp->gate.io[2].hashcode;
	}else {
		printf("error otype\n");
		exit(0);
	}
}

void printgate(Gate *g, int size) {
	if (!gate || size==0)	return;
	for (int i=0;i<size;++i) {
		printotype(g[i].otype);
		if (is_unaop(g[i].otype)) {
			printf("%s %s\n", g[i].io[0].key, g[i].io[2].key);
		}else if (is_binop(g[i].otype)) {
			printf("%s %s %s\n", g[i].io[0].key, g[i].io[1].key, g[i].io[2].key);
		}else if (is_decoder(g[i].otype)) {
			printf("insize[%d]\n", g[i].insize);
			for (int j=0;j<g[i].insize;++j)
				printf("%s ", g[i].in[j].key);
			printf("\noutsize[%d]\n", g[i].outsize);
			for (int j=0;j<g[i].outsize;++j)
				printf("%s ", g[i].out[j].key);
			printf("\n");
		}else if (is_multiplexer(g[i].otype)) {
			printf("size[%d]\n", g[i].size);
			printf("insize[%d]\n", g[i].insize);
			for (int j=0;j<g[i].insize;++j)
				printf("%s ", g[i].in[j].key);
			printf("\nselectsize[%d]\n", g[i].size);
			for (int j=0;j<g[i].size;++j)
				printf("%s ", g[i].select[j].key);
			printf("\nout %s\n", g[i].io[2].key);
		}
	}
}

void clear_glist()
{
	GNode *gnode;
	for (int i=0;i<glist.size;++i) {
		gnode = glist.head->next;
		free(glist.head);
		glist.head = gnode;
	}
	glist.last = NULL;
	glist.size = 0;
}

void set_tlist()
{	// collect all the temp variables
	tlist.head = NULL;
	tlist.last = NULL;
	tlist.size = 0;
	
	char *key;
	op_type otype;
	for (int i=0;i<gatesize;++i) {
		otype = gate[i].otype;
		if (is_unaop(otype) || is_binop(otype)) {
			key = gate[i].io[2].key;
			if (is_ignore(key) || Exist(key))
				continue;
			add_tlist(key);
		}else if (is_decoder(otype)) {
			int outsize = gate[i].outsize;
			for (int j=0;j<outsize;++j) {
				key = gate[i].out[j].key;
				if (is_ignore(key) || is_const(key) || Exist(key))
					continue;
				add_tlist(key);
			}
		}else if (is_multiplexer(otype)) {
			key = gate[i].io[2].key;
			if (is_ignore(key) || is_const(key) || Exist(key))
				continue;
			add_tlist(key);
		}else {
			printf("invalid otype\n");
			exit(0);
		}
	}
}

void add_tlist(char *key)
{
	TNode *tnode = (TNode*)calloc(1, sizeof(TNode));
	if (!tnode) {
		printf("failed to malloc for TNode, lineno=%d\n", __LINE__);
		exit(0);
	}
	strcpy(tnode->token.key, key);
	tnode->next = NULL;

	if (tlist.size)
		tlist.last->next = tnode;
	else
		tlist.head = tnode;
	tlist.last = tnode;
	++(tlist.size);
}

void print_tlist()
{
	if (!tlist.size || !tlist.head)	return;
	printf("printing tlist[size=%d]\n", tlist.size);
	TNode *tnode = tlist.head;
	for (int i=0;i<tlist.size;++i) {
		printf("%s ", tnode->token.key);
		tnode = tnode->next;
	}
	printf("\n");
}

void notemp_compute()
{
	int i = 0;
	int bound = 1<<inputsize;
	int j;
looplabel0:
	j = i;
	for (int iter=inputsize-1;iter>=0;--iter) {
		input[iter].value = j & 1;
		j >>= 1;
	}
	notemp_gate_compute();
	if (++i < bound)	goto looplabel0;

	// clean
	free(input);
	input = NULL;

	free(output);
	output = NULL;

	freegate();
	clear_table();
}

void notemp_gate_compute()
{
	op_type otype;
	for (int i=0;i<gatesize;++i) {
		otype = gate[i].otype;
		if (is_unaop(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;
			int in, out;
			char *key;
			key = gate[i].io[0].key;
			if (key[0]=='0')	in=0;
			else if (key[0]=='1')	in=1;
			else	in = Search(key);
			if (in!=0 && in!=1) {
				printf("invalid,in[%d], lineno=%d\n", in, __LINE__);
				exit(0);
			}
			if (otype == NOT)	out = !in;
			else	out = in;
			Update(gate[i].io[2].key, out);
			//gate[i].io[2].value = out; //unnecessary
		}else if (is_binop(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;
			int in[2], out;
			for (int j=0;j<2;++j) {
				if (gate[i].io[j].key[0]=='0')	in[j]=0;
				else if (gate[i].io[j].key[0]=='1')	in[j]=1;
				else	in[j] = Search(gate[i].io[j].key);
				if (in[j]!=0 && in[j]!=1) {
					printf("invalid, in[%d], lineno=%d\n", in[j], __LINE__);
					exit(0);
				}
			}
			switch (otype) {
				case AND:	out=in[0]&in[1];	break;
				case OR:	out=in[0]|in[1];	break;
				case NAND:
					if (in[0]==1 && in[1]==1)	out=0;
					else	out=1;
					break;
				case NOR:
					if (in[0]==0 && in[1]==0)	out=1;
					else	out=0;
					break;
				case XOR:	out=in[0]^in[1];	break;
				default:
					printf("invalid otype[%d], lineno=%d\n", otype, __LINE__);
					exit(0);
			}
			Update(gate[i].io[2].key, out);
			//gate[i].io[2].value = out; // unnecessary
		}else if (is_decoder(otype)) {
			int size = gate[i].size;
			int outsize = gate[i].outsize;
			char *instr = (char*)malloc(size+1);
			instr[size] = 0;
			int v = 0;
			for (int j=0;j<size;++j) {
				if (gate[i].in[j].key[0] == '0')	v = 0;
				else if (gate[i].in[j].key[0] == '1')	v = 1;
				else {
					v = Search(gate[i].in[j].key);
					if (v!=0 && v!=1) {
						printf("invalid search value[%d]\n", v);
						exit(0);
					}
				}
				instr[j] = '0' + v;
			}
			int out_index = binstrtoi(instr);
			for (int j=0;j<outsize;++j) {
				if (Exist(gate[i].out[j].key)) {
					if (j != out_index)
						gate[i].out[j].value = 0;
					else
						gate[i].out[j].value = 1;
					Update(gate[i].out[j].key, gate[i].out[j].value);
				}
			}
			free(instr);
		}else if (is_multiplexer(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;

			int size = gate[i].size;
			Token *select = gate[i].select;
			int select_index = 0;
			char *selectstr = (char*)malloc(size+1);
			if (!selectstr) {
				printf("failed to malloc for selectstr\n");
				exit(0);
			}
			selectstr[size] = 0;
			int v=0;
			for (int j=0;j<size;++j) {
				if (is_const(select[j].key))	v=select[j].value;
				else {
					v=Search(select[j].key);
					select[j].value = v;
				}
				if (v!=0 && v!=1) {
					printf("invalid v[%d], lineno=%d\n", v, __LINE__);
					exit(0);
				}
				selectstr[j] = '0' + v;
			}
			select_index = binstrtoi(selectstr);
			if (is_const(gate[i].in[select_index].key))	
				v = gate[i].in[select_index].value;
			else {
				v = Search(gate[i].in[select_index].key);
				gate[i].in[select_index].value = v;
			}
			if (v!=0 && v!=1) {
				printf("invalid v[%d], lineno=%d\n", v, __LINE__);
				exit(0);
			}
			gate[i].io[2].value = v;
			Update(gate[i].io[2].key, v);
			free(selectstr);
		}
	}
	// print out the current row of truthtable
	for (int i=0;i!=inputsize;++i)	printf("%d ", input[i].value);
	printf("|");
	for (int i=0;i<outputsize;++i)	printf(" %d", output[i].value);
	printf("\n");

}

int binstrtoi(char *str)
{
	if (!str || strlen(str)==0)	{
		printf("invalid str, lineno=%d\n", __LINE__);
		exit(0);
	}
	int len = strlen(str);
	int result = 0;
	for (int i=0;i!=len;++i) {
		result = (result<<1) + str[i]-'0';
	}
	return result;
}

void compute()
{
	int bound = 1<<inputsize;
	int i = 0;
	int j;
looplabel1:
	j = i++;
	for (int k=inputsize-1;k!=-1;--k) {
		input[k].value = j&1;
		j >>= 1;
	}
	gate_compute();
	if (i != bound)	goto looplabel1;

	// clean
	free(gatequeue);
	gatequeue = NULL;

	free(input);
	input = NULL;

	free(output);
	output = NULL;

	free(temp);
	temp = NULL;

	freegate();
	clear_table();

}

void gate_compute()
{	// according to gatequeue, to iterate the gate
	int  i, index;
	op_type otype;
	for (index=0;index!=gatesize;++index) {
		i = gatequeue[index];
		otype = gate[i].otype;
		if (is_unaop(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;

			int in;
			if (is_const(gate[i].io[0].key))	in=gate[i].io[0].value;
			else	in=Search(gate[i].io[0].key);
			if (in<0 || in>1) {
				printf("invalid in[%d],key=%s, lineno=%d\n", in, gate[i].io[0].key, __LINE__);
				exit(0);
			}
			int out;
			if (otype == NOT) out=!in;
			else if (otype == PASS)	out=in;
			else {
				printf("invalid otype[%d],lineno=%d\n", otype, __LINE__);
				exit(0);
			}
			Update(gate[i].io[2].key, out);
		}else if (is_binop(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;

			int in[2], out;
			for (int j=0;j<2;++j) {
				if (is_const(gate[i].io[j].key))	in[j] = gate[i].io[j].value;
				else	in[j] = Search(gate[i].io[j].key);
			}
			switch (otype) {
				case AND:
					out = in[0]&in[1];break;
				case OR:
					out = in[0]|in[1];break;
				case NAND:
					out = !(in[0]&in[1]);break;
				case NOR:
					out = !(in[0]|in[1]);break;
				case XOR:
					out = in[0]^in[1];break;
				default:
					printf("Invalid otype[%d]\n", otype);
					exit(0);
			}
			Update(gate[i].io[2].key, out);
		}else if (is_decoder(otype)) {
			int insize = gate[i].insize;
			int outsize = gate[i].outsize;
			char *instr = (char*)malloc(insize+1);
			if (!instr) {
				printf("failed to malloc for instr, lineno=%d\n", __LINE__);
				exit(0);
			}
			for (int j=0;j<insize;++j) {
				if (is_const(gate[i].in[j].key))
					instr[j] = '0' + gate[i].in[j].value;
				else
					instr[j] = '0' + Search(gate[i].in[j].key);
			}
			instr[insize] = 0;
			int target = binstrtoi(instr);
			free(instr);
			for (int j=0;j<outsize;++j) {
				if (!is_ignore(gate[i].out[j].key)) {
					if (j != target) Update(gate[i].out[j].key, 0);
					else	Update(gate[i].out[j].key, 1);
				}
			}
		}else if (is_multiplexer(otype)) {
			if (is_ignore(gate[i].io[2].key))	continue;

			int selectsize = gate[i].size;
			Token *in = gate[i].in;
			Token *select = gate[i].select;
			char *selectstr = (char*)malloc(selectsize+1);
			if (!selectstr) {
				printf("failed to malloc for selectstr, lineno=%d\n", __LINE__);
				exit(0);
			}
			for (int j=0;j<selectsize;++j) {
				if (is_const(select[j].key))	
					selectstr[j] = '0' + select[j].value;
				else
					selectstr[j] = '0' + Search(select[j].key);
			}
			selectstr[selectsize] = 0;
			int selectvalue = binstrtoi(selectstr);
			free(selectstr);
			int v;
			if (is_const(in[selectvalue].key))	v=in[selectvalue].value;
			else	v = Search(in[selectvalue].key);
			Update(gate[i].io[2].key, v);
		}else {
			printf("invalid otype[%d], lineno=%d\n", otype, __LINE__);
			exit(0);
		}
	}
	// printout current row of the truthtable
	for (int i=0;i<inputsize;++i)	printf("%d ", input[i].value);
	printf("|");
	for (int i=0;i<outputsize;++i)	printf(" %d", output[i].value);
	printf("\n");
}

void build_gatequeue() //set_reftable()
{
	// initialization.
	// at the end of this function, we also clean reftable and gatedep.
	reftable = (RefList*)calloc(gatesize, sizeof(RefList));
	if (!reftable) {
		printf("failed to calloc for reftable\n");
		exit(0);
	}
	gatedep = (GateDep*)calloc(gatesize, sizeof(GateDep));
	if (!gatedep) {
		printf("failed to calloc for gatedep, lineno=%d\n", __LINE__);
		exit(0);
	}
	// (1)for each temp var, search the OUTPUT of each gate, 
	// then we know where it is defined. It is the Reftable bucket
	// (2)search this temp var in the INPUT and SELECT parts in each gate,
	// then we attain its ref information
	for (int i=0;i<tempsize;++i) {
		char *key = temp[i].key;
		int hashcode = temp[i].hashcode;
		int bucket = -1; // inside which gate it is defined.
		for (int j=0;j<gatesize;++j) {  // (1)
			if (is_unaop(gate[j].otype)) {
				if (hashcode==gate[j].io[2].hashcode
					&& strcmp(key, gate[j].io[2].key)==0) {
					bucket = j;
					break;
				}
			}else if (is_binop(gate[j].otype)) {
				if (hashcode==gate[j].io[2].hashcode
					&& strcmp(key, gate[j].io[2].key)==0) {
					bucket = j;
					break;
				}
			}else if (is_decoder(gate[j].otype)) {
				for (int i=0;i<gate[j].outsize;++i) {
					if (hashcode==gate[j].out[i].hashcode
						&& strcmp(key, gate[j].out[i].key)==0) {
						bucket = j;
						break;
					}
				}
				if (bucket != -1)	break;
			}else if (is_multiplexer(gate[j].otype)) {
				if (hashcode==gate[j].io[2].hashcode
					&& strcmp(key, gate[j].io[2].key)==0) {
					bucket = j;
					break;
				}
			}else {
				printf("invalid gatetye[%d], gate_index[%d], lineno=%d\n",
						gate[j].otype, j, __LINE__);
				exit(0);
			}
		} // end of loop for the OUTPUT part of gate iteration
		if (bucket==-1) {
			printf("invalid date file, failed to find the definition of %s, tempindex=%d, lineno=%d\n", key, i, __LINE__);
			exit(0);
		}

		// (2)
		for (int j=0;j<gatesize;++j) {
			if (j != bucket) {
				if (ExistReftable(bucket, j)) // if already exist,unnecessary 
					continue;

				if (is_unaop(gate[j].otype)) {
					if (hashcode==gate[j].io[0].hashcode
						&& strcmp(key, gate[j].io[0].key)==0)
						InsertReftable(bucket, j);
				}else if (is_binop(gate[j].otype)) {
					for (int i=0;i<2;++i)
						if (hashcode==gate[j].io[i].hashcode
							&& strcmp(key, gate[j].io[i].key)==0) {
							InsertReftable(bucket, j);
							break;
						}
				}else if (is_decoder(gate[j].otype)) {
					for (int i=0;i<gate[j].insize;++i)
						if (hashcode==gate[j].in[i].hashcode
							&& strcmp(key, gate[j].in[i].key)==0) {
							InsertReftable(bucket, j);
							break;
						}
				}else if (is_multiplexer(gate[j].otype)) {
					int found = 0;
					for (int i=0;i<gate[j].size;++i) {
						if (hashcode==gate[j].select[i].hashcode
							&& strcmp(key, gate[j].select[i].key)==0) {
							InsertReftable(bucket, j);
							found = 1;
							break;
						}
					}
					if (!found) {
						for (int i=0;i<gate[j].insize;++i) 
							if (hashcode==gate[j].in[i].hashcode
								&& strcmp(key, gate[j].in[i].key)==0) {
								InsertReftable(bucket, j);
								break;
							}
					}
				}else {
					printf("invalid gatetye[%d], gate_index[%d], lineno=%d\n",
							gate[j].otype, j, __LINE__);
					exit(0);
				}
			}
		} // end of loop for INPUT part of gate iteration

	} // end of loop for tempvar iteration
	
	// build gatequeue. After gatequeue is built, we should clear reftable and gatedep immediately.
	gatequeue = (int*)calloc(gatesize, sizeof(int));
	if (!gatequeue) {
		printf("failed to calloc for gatequeue,lineno=%d\n", __LINE__);
		exit(0);
	}
	int i=0;
	while (i!=gatesize) {
		for (int j=0;j!=gatesize;++j) {
			if (!gatedep[j].mark && !gatedep[j].degree) { // j enter gatequeue
				gatedep[j].mark = 1;
				gatequeue[i++] = j;
				if (reftable[j].head) { // clean reftable[j]
					int k;
					if (reftable[j].head->next == NULL) {
						k = reftable[j].head->gate_index;
						gatedep[k].degree--;
						free(reftable[j].head);
						reftable[j].head = NULL;
					}else {
						RefNode *current = reftable[j].head->next;
						while (current) {
							k = current->gate_index;
							gatedep[k].degree--;
							reftable[j].head->next = current->next;
							free(current);
							current = reftable[j].head->next;
						}
						k = reftable[j].head->gate_index;
						gatedep[k].degree--;
						free(reftable[j].head);
						reftable[j].head = NULL;
					}
				} //end for cleaning reftable[j]
			} // end for j enter gatequeue
		} // end loop for gatedep
	} // if i!=gatesize, means there is still space in gatequeue, continue
	// print gatequeue for debug Correct!
	/*
	for (int i=0;i<gatesize;++i)
		printf("%d\t", gatequeue[i]);
	printf("\n");
	*/
	free(reftable);
	reftable = NULL;
	free(gatedep);
	gatedep = NULL;
} // end of build_gatequeue()

void InsertReftable(int bucket, int gate_index)
{	/*
	if (bucket<0 || bucket>=gatesize
		|| gate_index<0 || gate_index>=gatesize) {
		printf("Invalid InsertReftable, bucket[%d] gate_index[%d]\n",
				bucket, gate_index);
		exit(0);
	}
	*/
	/*
	while (current) {
		if (current->gate_index > gate_index)	break;
		else if (current->gate_index == gata_index)	return;
		current = current->next;
	}
	*/// we have already called ExistReftable(bucket, gate_index) before
	// calling InsertReftable(bucket, gate_index)
	++(gatedep[gate_index].degree);
	RefNode *new, *current=reftable[bucket].head;
	new = (RefNode*)calloc(1, sizeof(RefNode));
	new->gate_index = gate_index;

	if (!current)//(!reftable[bucket].head) // empty bucket
		reftable[bucket].head = new;
	else {
		/*
		current = reftable[bucket].head->next;
		reftable[bucket].head->next = new; // just insert next to the head
		new->next = current;
		*/
		//current = reftable[bucket].head;
		reftable[bucket].head = new;
		new->next = current;
		if (gate_index < current->gate_index)	return;
		while (!current && gate_index>current->gate_index) {
			new->gate_index = current->gate_index;
			new = current;
			current = current->next;
		}
		new->gate_index = gate_index;
	}
}

int ExistReftable(int bucket, int gate_index)
{
	if (bucket<0 || bucket>=gatesize
		|| gate_index<0 || gate_index>=gatesize) {
		printf("invalid bucket[%d] and gatesize[%d]\n", bucket, gatesize);
		exit(0);
	}

	RefNode *current = reftable[bucket].head;
	while (current) {
		if (current->gate_index==gate_index)	return 1;
		else if (current->gate_index>gate_index)	return 0;
		current = current->next;
	}
	return 0;
}

void set_tarr()
{
	tempsize = tlist.size;
	temp = (Token*)calloc(tempsize, sizeof(Token));
	if (!temp) {
		printf("failed to calloc for temp,lineno=%d\n", __LINE__);
		exit(0);
	}
	TNode *current;
	for (int i=0;i<tempsize;++i) {
		temp[i].value = -1;
		current = tlist.head;
		strcpy(temp[i].key, current->token.key);
		temp[i].hashcode = Hash(temp[i].key);
		tlist.head = tlist.head->next;
		free(current);
	}
	tlist.head = NULL;
	tlist.last = NULL;
	tlist.size = 0;

	set_temp_table();
}

void set_temp_table()
{
	for (int i=0;i!=tempsize;++i)
		Insert(temp[i].key, TEMP, i);
}

void deal_input()
{
	// INPUT line
	fscanf(fp, " %16s ", inputtoken);
	if (strcmp(inputtoken, "INPUT")!=0) {
		printf("invalid data\n");
		exit(0);
	}
	fscanf(fp, " %16s ", inputtoken);
	inputsize = atoi(inputtoken);
	if (inputsize<1) {
		printf("invalid inputsize[%d]\n", inputsize);
		exit(0);
	}
	//printf("input size=%d\n", inputsize);
	input = (Token*)malloc(sizeof(Token)*inputsize);
	if (!input) {
		printf("failed to malloc for input\n");
		exit(0);
	}
	for (int i=0;i<inputsize;++i) {
		fscanf(fp, " %16s ", inputtoken);
		input[i].value = -1;
		strcpy(input[i].key, inputtoken);
		input[i].hashcode = Hash(inputtoken);
	}
	//printarr(input, inputsize);
	// INPUT line ends

}

void deal_output()
{
	// OUTPUT line
	fscanf(fp, " %16s ", inputtoken);
	if (strcmp(inputtoken, "OUTPUT")!=0) {
		printf("OUTPUT is missing\n");
		exit(0);
	}
	fscanf(fp, " %16s ", inputtoken);
	outputsize = atoi(inputtoken);
	if (outputsize<1) {
		printf("output size[%d] is invalid\n", outputsize);
		exit(0);
	}
	output = (Token*)malloc(sizeof(Token)*outputsize);
	if (!output) {
		printf("malloc for outputarr failed\n");
		exit(0);
	}
	for (int i=0;i<outputsize;++i) {
		output[i].value = -1;
		fscanf(fp, " %16s ", inputtoken);
		strcpy(output[i].key, inputtoken);
		output[i].hashcode = Hash(inputtoken);
	}
	//printarr(output, outputsize);
}

void deal_gate()
{
	// Read all the OPERATION part
	glist.head = NULL;
	glist.last = NULL;
	glist.size = 0;
	if (fscanf(fp, " %16s ", inputtoken)!=1) {
		printf("No operation!\n");
		exit(0);
	}
	GNode *current_gnode;
	op_type tmp_otype;
	do {
		current_gnode = (GNode*)calloc(1, sizeof(GNode)); //(GNode*)malloc(sizeof(GNode));
		if (!current_gnode) {
			printf("failed to malloc for current_gnode\n");
			exit(0);
		}
		current_gnode->next = NULL;
		tmp_otype = get_optype(inputtoken);
		current_gnode->gate.otype = tmp_otype;
		if (is_unaop(tmp_otype)) {
			// in
			fscanf(fp, " %16s ", inputtoken);
			strcpy(current_gnode->gate.io[0].key, inputtoken);
			if (is_const(inputtoken))	
				current_gnode->gate.io[0].value = atoi(inputtoken);
			else
				current_gnode->gate.io[0].value = -1;
			current_gnode->gate.io[0].hashcode = Hash(inputtoken);

			// out
			fscanf(fp, " %16s ", inputtoken);
			strcpy(current_gnode->gate.io[2].key, inputtoken);
			current_gnode->gate.io[2].value = -1;
			current_gnode->gate.io[2].hashcode = Hash(inputtoken);
		}else if (is_binop(tmp_otype)) {
			// in1
			fscanf(fp, " %16s ", inputtoken);
			strcpy(current_gnode->gate.io[0].key, inputtoken);
			if (is_const(inputtoken))
				current_gnode->gate.io[0].value = atoi(inputtoken);
			else
				current_gnode->gate.io[0].value = -1;
			current_gnode->gate.io[0].hashcode = Hash(inputtoken);
			
			// in2
			fscanf(fp, " %16s ", inputtoken);
			strcpy(current_gnode->gate.io[1].key, inputtoken);
			if (is_const(inputtoken))
				current_gnode->gate.io[1].value = atoi(inputtoken);
			else
				current_gnode->gate.io[1].value = -1;
			current_gnode->gate.io[1].hashcode = Hash(inputtoken);
		
			// out
			fscanf(fp, " %16s ", inputtoken);
			strcpy(current_gnode->gate.io[2].key, inputtoken);
			current_gnode->gate.io[2].value = -1;
			current_gnode->gate.io[2].hashcode = Hash(inputtoken);
		}else if (is_decoder(tmp_otype)) {
			// size
			fscanf(fp, " %16s ", inputtoken);
			int tmp_size = atoi(inputtoken);
			current_gnode->gate.size = tmp_size;
			current_gnode->gate.insize = tmp_size;
			current_gnode->gate.outsize = 1<<tmp_size;
			// in, tmp_size
			current_gnode->gate.in = (Token*)calloc(tmp_size, sizeof(Token));
			//(Token*)malloc(sizeof(Token)*tmp_size);
			if (!(current_gnode->gate.in)) {
				printf("failed to malloc for gate.in\n");
				exit(0);
			}
			for (int i=0;i<tmp_size;++i) {
				fscanf(fp, " %16s ", inputtoken);
				strcpy(current_gnode->gate.in[i].key, inputtoken);
				if (is_const(inputtoken))
					current_gnode->gate.in[i].value = atoi(inputtoken);
				else
					current_gnode->gate.in[i].value = -1;
				current_gnode->gate.in[i].hashcode = Hash(inputtoken);
			}
			// out, current_gnode->gate.outsize = 1<<tmp_size;
			current_gnode->gate.out = 
				(Token*)calloc(current_gnode->gate.outsize, sizeof(Token));
			
			if (!(current_gnode->gate.out)) {
				printf("failed to malloc for gate.out, lineno=%d\n", __LINE__);
				exit(0);
			}
			for (int i=0;i<current_gnode->gate.outsize;++i) {
				fscanf(fp, " %16s ", inputtoken);
				strcpy(current_gnode->gate.out[i].key, inputtoken);
				current_gnode->gate.out[i].value = -1;
				current_gnode->gate.out[i].hashcode = Hash(inputtoken);
			}

		}else if (is_multiplexer(tmp_otype)) {
			// size
			fscanf(fp, " %16s ", inputtoken);
			int tmp_size = atoi(inputtoken);
			current_gnode->gate.size = tmp_size;
			current_gnode->gate.insize = 1<<tmp_size;
			current_gnode->gate.in = 
				(Token*)calloc(current_gnode->gate.insize, sizeof(Token));
			if (!(current_gnode->gate.in)) {
				printf("failed to malloc for gate.in, lineno=%d\n", __LINE__);
				exit(0);
			}
			// in, current_gnode->insize = 1<<tmp_size;
			for (int i=0;i<current_gnode->gate.insize;++i) {
				fscanf(fp, " %16s ", inputtoken);
				strcpy(current_gnode->gate.in[i].key, inputtoken);
				if (is_const(inputtoken))
					current_gnode->gate.in[i].value = atoi(inputtoken);
				else
					current_gnode->gate.in[i].value = -1;
				current_gnode->gate.in[i].hashcode = Hash(inputtoken);
			}
			// select, size=tmp_size
			current_gnode->gate.select =
				(Token*)calloc(tmp_size, sizeof(Token));
			if (!(current_gnode->gate.select)) {
				printf("failed to malloc for gate.select\n");
				exit(0);
			}
			for (int i=0;i<tmp_size;++i) {
				fscanf(fp, " %16s ", inputtoken);
				strcpy(current_gnode->gate.select[i].key, inputtoken);
				if (is_const(inputtoken))
					current_gnode->gate.select[i].value = atoi(inputtoken);
				else
					current_gnode->gate.select[i].value = -1;
				current_gnode->gate.select[i].hashcode = Hash(inputtoken);
			}
			// out
			fscanf(fp, " %16s ", inputtoken);
			current_gnode->gate.io[2].value = -1;
			strcpy(current_gnode->gate.io[2].key, inputtoken);
			current_gnode->gate.io[2].hashcode = Hash(inputtoken);

		}
		if (!glist.size)   // size==0
			glist.head = current_gnode;
		else
			glist.last->next = current_gnode;

		glist.last = current_gnode;
		++glist.size;
	}while(fscanf(fp, " %16s ", inputtoken)==1);
	// store the operation lines into arr
	gatesize = glist.size;
	gate = (Gate*)calloc(gatesize, sizeof(Gate));
	if (!gate) {
		printf("failed to malloc for gate\n");
		exit(0);
	}
	current_gnode = glist.head;
	for (int i=0;i<gatesize;++i) {
		copygate(gate+i, current_gnode);
		current_gnode = current_gnode->next;
	}
	clear_glist();

}

void set_io_table()
{
	// Set INPUT and OUTPUT in the Table
	for (int i=0;i<SIZE;++i)	Table[i] = NULL;
	for (int i=0;i<inputsize;++i)	Insert(input[i].key, INPUT, i);
	for (int i=0;i<outputsize;++i)	Insert(output[i].key, OUTPUT, i);
}

void freegate()
{
	for (int i=0;i<gatesize;++i) {
		if (gate[i].in)	free(gate[i].in);
		if (gate[i].out)	free(gate[i].out);
		if (gate[i].select)	free(gate[i].select);
	}
	free(gate);
}

void clear_table()
{
	Node *current, *head;
	for (int i=0;i<SIZE;++i) {
		if (Table[i]) {
			head = Table[i];
			while (head->next) {
				current = head->next->next;
				free(head->next);
				head->next = current;
			}
			free(head);
			Table[i] = NULL;
		}
	}
}

