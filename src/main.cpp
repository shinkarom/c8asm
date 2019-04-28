#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

stringstream ss(stringstream::binary|stringstream::out);

unordered_map<string, int> symtable;
unordered_map<string, int> regtable;
unordered_map<int, string> labelrefs;

int addr;

string curstr;
int line_num;
size_t pos;
bool is_eof;

void init_tokenizer(string& arg)
{
	curstr = arg;
	pos = 0;
	is_eof = false;		
}

int convert_todec(const string& input, int base)
{
	if(base < 2 || base > 36)
		return 0;		   
	int startIndex = input.length()-1;		
	int value = 0;
	int digitValue = 1;		
	for(int i = startIndex; i >= 0; --i){
		char c = input[i];			
		if(c >= 'a' && c <= 'z')
			c -= ('a' - 'A');			
		if(c >= '0' && c <= '9')
			c -= '0';
		else
			c = c - 'A' + 10;		
		if(c >= base)
			return 0;			   
		value += c * digitValue;			
		digitValue *= base;
	}				
	return value;
}

void limit_number(int n,int limit)
{
	if(n>limit){
		cout<<"Line "<<line_num<<": Number too large"<<endl;
		exit(1);
	}
}

bool is_bindigit(char c)
{
	return (c=='0')|(c=='1');	
}

bool is_octdigit(char c)
{
	const char octdigits[] = {'0','1','2','3','4','5','6','7'};
	return find(begin(octdigits),end(octdigits),c)!=end(octdigits);		
}

bool is_hexdigit(char c)
{
	const char hexdigits[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	return find(begin(hexdigits),end(hexdigits),c)!=end(hexdigits);		
}

bool is_register(string s)
{
	return (s.length()==2)&&(s[0]=='v')&&(is_hexdigit(s[1]));
}

bool is_registerthing(string s)
{
	if(is_register(s))
		return true;
	else if(regtable.find(s)!=regtable.end())
		return true;
	else return false;
}

bool is_decdigit_nonzero(char c)
{
	const char decdigits_nonzero[] = {'1','2','3','4','5','6','7','8','9'};
	return find(begin(decdigits_nonzero),end(decdigits_nonzero),c)!=end(decdigits_nonzero);		
}

bool is_decdigit(char c)
{
	const char decdigits[] = {'0','1','2','3','4','5','6','7','8','9'};
	return find(begin(decdigits),end(decdigits),c)!=end(decdigits);		
}

bool is_whitespace(char c)
{
	const char whitespace[] = {' ','\t'};
	return find(begin(whitespace),end(whitespace),c)!=end(whitespace);
}

bool is_reserved(string s)
{
	const vector<string> reserved = {"cls","ret","sys","jp","call","se","sne","ld","add","or","and","xor","sub","shr","subn","shl","sne","rnd","drw","skp","sknp","scd","scr","scl","exit","low","high","i","dt","st","f","b","v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","va","vb","vc","vd","ve","vf","db","dw","k"};
	return find(begin(reserved),end(reserved),s)!=end(reserved);
}

bool is_number(string s)
{
	if(is_decdigit_nonzero(s[0])){
		for(size_t i = 1; i<s.length();i++){
			if(!is_decdigit(s[i]))
				return false;
		}
		return true;
	} else
	if(s[0]=='$'){
		for(size_t i = 1; i<s.length();i++){
			if(!is_hexdigit(s[i]))
				return false;
		}
		return true;
	} else	
	if(s[0]=='%'){
		for(size_t i = 1; i<s.length();i++){
			if(!is_bindigit(s[i]))
				return false;
		}
		return true;
	} else		
	if(s[0]=='0'){
		for(size_t i = 1; i<s.length();i++){
			if(!is_octdigit(s[i]))
				return false;
		}
		return true;
	} else return false;
}

bool is_numberthing(string s)
{
	return is_number(s)||(symtable.find(s)!=symtable.end());
}

void skip_whitespace()
{
	while(is_whitespace(curstr[pos]))
		pos++;
	is_eof = pos==curstr.length();
	//cout<<"skipped whitespace from "<<old_pos<<" to "<<pos<<endl;
}

void expect_whitespace()
{
	if(!is_whitespace(curstr[pos])){
		cout<<"Whitespace expected."<<endl;
		exit(1);
	}
	skip_whitespace();
}

string get_token(bool do_peek = false)
{
	skip_whitespace();
	size_t prev_pos = pos;
	pos = curstr.find_first_of(" \t,",pos);
	if(pos==string::npos){
		pos = curstr.length();
		is_eof = true;
	}
	auto g = curstr.substr(prev_pos,pos-prev_pos);	
	transform(g.begin(),g.end(),g.begin(),::tolower);	
	//cout<<"got token \""<<g<<"\""<<endl;
	return g;
}

string peek_token()
{
	size_t prev_pos = pos;
	auto k = get_token(true);
	//cout<<"peeked at token \""<<k<<"\""<<endl;
	pos = prev_pos;
	return k;
}	

bool optional_comma()
{
	int old_pos = pos;
	skip_whitespace();
	if(curstr[pos]==','){
		pos++;
		return true;
	}
	pos=old_pos;
	return false;
}

void expect_comma()
{
	skip_whitespace();
	if(curstr[pos]!=','){
		cout<<"Comma expected"<<endl;
		exit(0);
	}
	else {
		pos++;
	}
}

int get_register()
{
	auto tk = get_token();
	auto n = convert_todec(tk.substr(1),16);
	return n;
}	

int expect_register()
{
	auto tk = peek_token();
	if(!is_register(tk)){
		cout<<"Register expected, but got "<<tk<<endl;
		exit(0);
	}
	else return get_register();
}

int expect_registerthing()
{
	auto tk = peek_token();
	if(is_register(tk))
		return get_register();
	else if(auto r = regtable.find(tk);r!=regtable.end()){
		get_token();
		return r->second;
	}else{
		cout<<"Registerthing expected, but got "<<tk<<endl;
		exit(1);
	}
}

int get_number()
{
	auto tk = get_token();
	if(tk[0]=='%')
		return convert_todec(tk.substr(1),2); else
	if(tk[0]=='$')
		return convert_todec(tk.substr(1),16); else
	if(tk[0]=='0')
		return convert_todec(tk.substr(1),8); else
	return stoi(tk);
}

int expect_numberthing()
{
	int result;
	auto tk = peek_token();
	if(!is_number(tk)){
		if(auto it=symtable.find(tk);it!=symtable.end()){
			result = it->second;
			get_token();
		} else{
			cout<<"Numberthing expected, but got "<<tk<<endl;
			exit(0);
		}
	} else	
		result = get_number();
	cout<<"got numberthing "<<result<<endl;
	return result;
}

int expect_address(){
	auto n = expect_numberthing();
	limit_number(n,4192);
	return n;
}

void emit_twobytes(int hi, int lo)
{
	ss.write((char*)&hi,1);
	ss.write((char*)&lo,1);
}

void emit_oneword(int wrd)
{
	emit_twobytes(wrd>>8,wrd % 256);
}

void process_one_db()
{
	int r=expect_numberthing();
	limit_number(r,255);
	ss.write((char*)&r,1);
	addr++;
}

void process_db()
{
	expect_whitespace();
	process_one_db();
	while(optional_comma())
		process_one_db();
	cout<<"processed db"<<endl;	
}

void process_one_dw()
{
	int r = expect_numberthing();
	limit_number(r,65535);
	emit_oneword(r);
	addr+=2;	
}

void process_dw()
{
	expect_whitespace();
	process_one_dw();
	while(optional_comma())
		process_one_dw();		
	cout<<"processed dw"<<endl;	
}

void process_ld_i()
{
	get_token();
	expect_comma();
	auto rr = peek_token();
	if(is_registerthing(rr)){
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,0x55);
	} else if(is_numberthing(rr)){
		auto r = expect_address();
			emit_oneword(0xA000|r);
	} else{
		cout<<"Wrong argument."<<endl;
		exit(1);
	}
}

void process_ld_vx()
{
	const unordered_map<string, int> not_register_second = {{"dt",0x07},{"k",0x0A},{"i",0x65}};
	auto x = expect_registerthing();
	expect_comma();	
	auto y = peek_token();
	if(auto o = not_register_second.find(y);o!=not_register_second.end()){
		emit_twobytes(0xF0|x,o->second);
		get_token();
	}		
	else if(is_registerthing(y)){
		auto yy = expect_registerthing();
		emit_twobytes(0x80|x,0x00|(yy<<4));
	} else if(is_number(y)){
		auto yy = get_number();
		limit_number(yy,255);
		emit_twobytes(0x60|x,yy);
	}
}

void process_ld()
{
	const unordered_map<string, int> not_register_first = {{"dt",0x15},{"st",0x18},{"f",0x29},{"b",0x33}};
	auto mn = peek_token();
	if(is_registerthing(mn))
		process_ld_vx();
	else if(auto o = not_register_first.find(mn);o!=not_register_first.end()){
		get_token();
		expect_comma();
		auto r = expect_registerthing();
		emit_twobytes(0xF0|r,o->second);		
	}
	else if(mn=="i")
		process_ld_i();
}

void process_mnemonic()
{
	const unordered_map<string,int> two_arg_mnemonics = {{"or",0x8001},{"and",0x8002},{"xor",0x8003},{"sub",0x8005},{"subn",0x8007},{"se",0x5000}};
	auto mnemonic = get_token();
	if (!is_reserved(mnemonic)) return;	
	if(auto o=two_arg_mnemonics.find(mnemonic);o!=two_arg_mnemonics.end()){
		auto x = expect_registerthing();
		expect_comma();
		auto y = expect_registerthing();
		emit_oneword(o->second|(x<<8)|(y<<4));
		addr+=2;
	}
	if(mnemonic=="db")	
		process_db();
	else if(mnemonic=="dw")
		process_dw();
	else if (mnemonic=="cls"){
		emit_twobytes(0x00,0xE0);
		addr+=2;
	}
	else if (mnemonic=="ret"){
		emit_twobytes(0x00,0xEE);
		addr+=2;		
	}	
	else if (mnemonic=="ld"){
		expect_whitespace();
		process_ld();
		addr+=2;
	}
	cout<<"mnemonic "<<mnemonic<<endl;
	auto tk = get_token();
	if(tk!=""){
		cout<<"Extraneous token "<<tk<<" on the line."<<endl;
		exit(0);
	}		
}

void process_line(string s)
{
	if(s.size()>0){
		init_tokenizer(s);
		auto tok1 = peek_token();
		if(is_reserved(tok1))			
			process_mnemonic();
		else {
			get_token();	
			if(auto tok2 = peek_token(); tok2=="equ"){		
				get_token();
				int i;
				auto tok3 = peek_token();
				if(is_number(tok3)){	
				i = get_number();
					symtable[tok1] = i;
				} else if(is_register(tok3)){
					i=get_register();
					regtable[tok1] = i;
				} else{
					cout<<"Wrong constant."<<endl;
					exit(1);
				}			
				cout<<"symbol "<<tok1<<" = "<<i<<endl;	
			}
			else {
				symtable[tok1] = addr;
				cout<<"label "<<tok1<<" = "<<addr<<endl;	
				process_mnemonic();			
			}			
		}
	}	
}

int main(int argc, char *argv[])
{
	ifstream ifs("test.asm",ios::in);
	char templine[256];
	addr = 0x200;
	line_num = 1;
	while(ifs.getline(templine,256))
	{		
		string thisline(templine);
		thisline = thisline.substr(0,thisline.find(";",0));
		process_line(thisline);
		line_num++;
	}
	ofstream ofs("test.c8",ios::binary|ios::out|ios::trunc);
	ofs<<ss.str();
	ofs.close();
	return 0;
}