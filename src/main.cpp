#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

stringstream ss(stringstream::binary|stringstream::out|stringstream::in);

unordered_map<string, int> symtable;
unordered_map<string, int> regtable;
unordered_map<int, string> labelrefs;

int addr;

string curstr;
int line_num;
size_t pos;
bool is_eof;

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
	string octdigits = "01234567";
	return octdigits.find(c)!=string::npos;		
}

bool is_hexdigit(char c)
{
	string hexdigits = "0123456789ABCDEF";
	return hexdigits.find(c)!=string::npos;		
}

bool is_register(string s)
{
	return (s.length()==2)&&(s[0]=='V')&&(is_hexdigit(s[1]));
}

bool is_register_or_symbol(string s)
{
	return is_register(s)||(regtable.find(s)!=regtable.end());
}

bool is_decdigit_nonzero(char c)
{
	string decdigits_nonzero = "123456789";
	return decdigits_nonzero.find(c)!=string::npos;		
}

bool is_decdigit(char c)
{
	string decdigits = "0123456789";
	return decdigits.find(c)!=string::npos;		
}

bool is_whitespace(char c)
{
	string whitespace = " \t";
	return whitespace.find(c)!=string::npos;
}

bool is_identchar(char c)
{
	const string identchars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	return identchars.find(c)!=string::npos;	
}

bool is_reserved(string s)
{
	const string reserved[] = {"CLS","RET","SYS","JP","CALL","SE","SNE","LD","ADD","OR","AND","XOR","SUB","SHR","SUBN","SHL","SNE","RND","DRW","SKP","SKNP","I","DT","ST","F","B","V0","V1","V2","V3","V4","V5","V6","V7","V8","V9","VA","VB","VC","VD","VE","VF","DB","DW","K"};
	return find(begin(reserved),end(reserved),s)!=end(reserved);
}

bool is_ident(string s)
{
	for(auto c : s)
		if(!is_identchar(c))
			return false;
	if(is_decdigit(s[0])) return false;
	return true;
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

bool is_number_or_label(string s)
{
	return is_number(s)||is_ident(s);
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

string get_token()
{
	string delims = " ,\t";
	string g = "";
	skip_whitespace();
	do{
		if(pos>=curstr.length()){
			is_eof=true;
			break;
		} 
		else if(delims.find(curstr[pos])!=string::npos)
			break;
		else{
			g+=toupper(curstr[pos]);
			pos++;
		}
	}while(true);
	//cout<<"got token \""<<g<<"\""<<endl;
	return g;
}

string peek_token()
{
	size_t prev_pos = pos;
	auto k = get_token();
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
	else pos++;
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

int expect_register_or_symbol()
{
	auto tk = peek_token();
	if(is_register(tk))
		return get_register();
	else if(auto r = regtable.find(tk);r!=regtable.end()){
		get_token();
		return r->second;
	}else{
		cout<<"register_or_symbol expected, but got "<<tk<<endl;
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

int expect_number_or_label()
{
	int result;
	auto tk = peek_token();
	if(is_number(tk))
		result = get_number();
	else if(auto it=symtable.find(tk);it!=symtable.end()){
			result = it->second;
			get_token();
	} else{
			cout<<"number_or_label expected, but got "<<tk<<endl;
			exit(0);
	}			
	//cout<<"got number_or_label "<<result<<endl;
	return result;
}

int expect_address(){
	auto t = peek_token();
	if(is_number(t)){
		auto n = get_number();
		limit_number(n,4192);
		return n;
	} else if(is_ident(t)&&(!is_reserved(t)))	{
		get_token();
		labelrefs[addr] = t;
		return 0;
	} else {
		cout<<"Address expected"<<endl;
		exit(1);
	}
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
	int r=expect_number_or_label();
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
	//cout<<"processed db"<<endl;	
}

void process_one_dw()
{
	int r = expect_number_or_label();
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
	//cout<<"processed dw"<<endl;	
}

void process_ld_vx()
{
	const unordered_map<string, int> register_first = {{"DT",0x07},{"K",0x0A},{"[I]",0x65}};
	auto x = expect_register_or_symbol();
	expect_comma();	
	auto y = peek_token();
	if(auto o = register_first.find(y);o!=register_first.end()){
		emit_twobytes(0xF0|x,o->second);
		get_token();
	}		
	else if(is_register_or_symbol(y)){
		auto yy = expect_register_or_symbol();
		emit_twobytes(0x80|x,0x00|(yy<<4));
	} else if(is_number(y)){
		auto yy = get_number();
		limit_number(yy,255);
		emit_twobytes(0x60|x,yy);
	}
}

void process_ld()
{
	const unordered_map<string, int> register_second = {{"DT",0x15},{"ST",0x18},{"F",0x29},{"B",0x33},{"[I]",0x55}};
	auto mn = peek_token();
	if(is_register_or_symbol(mn))
		process_ld_vx();
	else if(auto o = register_second.find(mn);o!=register_second.end()){
		get_token();
		expect_comma();
		auto r = expect_register_or_symbol();
		emit_twobytes(0xF0|r,o->second);		
	}
	else if(mn=="I"){
		get_token();
		expect_comma();
		auto r = expect_address();
		emit_oneword(0xA000|r);
	}		
}

void process_add()
{
	auto tok1 = peek_token();
	if(tok1=="i")
	{
		get_token();
		expect_comma();
		auto x = expect_register_or_symbol();
		emit_oneword(0xF01E|(x<<8));
	} else{
		auto x = expect_register_or_symbol();
		expect_comma();
		auto tok2 = peek_token();
		if(is_number_or_label(tok2)){
			auto n =expect_number_or_label();
			limit_number(n,255);
			emit_twobytes(0x70|x,n);
		}
		else{
			auto y = expect_register_or_symbol();
			emit_twobytes(0x80|x,0x04|(y<<4));
		}
	}
	addr+=2;
}

void process_jp()
{
	auto t = peek_token();
	if(t=="V0"){
		get_token();
		expect_comma();
		auto a = expect_address();
		emit_oneword(0xB000|a);
	} else {
		auto a = expect_address();
		emit_oneword(0x1000|a);		
	}
	addr+=2;
}

void process_sne()
{
	auto x = expect_register_or_symbol();
	expect_comma();
	auto tok2 = peek_token();
	if(is_register_or_symbol(tok2))
	{
		auto y = expect_register_or_symbol();
		emit_twobytes(0x90|x,0x00|(y<<4));
	} else if(is_number_or_label(tok2))
	{
		auto n = expect_number_or_label();
		limit_number(n,255);
		emit_twobytes(0x40|x,n);
	} else {
		cout<<"Expected either byte or register"<<endl;
		exit(1);
	}
}

void process_se()
{
	auto x = expect_register_or_symbol();
	expect_comma();
	auto tok2 = peek_token();
	if(is_register_or_symbol(tok2))
	{
		auto y = expect_register_or_symbol();
		emit_twobytes(0x50|x,0x00|(y<<4));
	} else if(is_number_or_label(tok2))
	{
		auto n = expect_number_or_label();
		limit_number(n,255);
		emit_twobytes(0x30|x,n);
	} else {
		cout<<"Expected either byte or register"<<endl;
		exit(1);
	}	
}

void process_mnemonic()
{
	const unordered_map<string,int> no_arg_mnemonics = {{"CLS",0x00E0},{"RET",0x00EE}};
	const unordered_map<string,int> one_reg_mnemonics = {{"SHR",0x8006},{"SHL",0x800E},{"SKP",0xE09E},{"SKNP",0xE0A1}};
	const unordered_map<string,int> two_arg_mnemonics = {{"OR",0x8001},{"AND",0x8002},{"XOR",0x8003},{"SUB",0x8005},{"SUBN",0x8007}};
	auto mnemonic = get_token();
	if (!is_reserved(mnemonic)) return;	
	if(auto o=two_arg_mnemonics.find(mnemonic);o!=two_arg_mnemonics.end()){
		auto x = expect_register_or_symbol();	
		expect_comma();
		auto y = expect_register_or_symbol();
		emit_oneword(o->second|(x<<8)|(y<<4));
		addr+=2;
	}
	else if(auto o=one_reg_mnemonics.find(mnemonic);o!=one_reg_mnemonics.end()){
		auto x = expect_register_or_symbol();	
		emit_oneword(o->second|(x<<8));
		addr+=2;
	}
	else if(auto o=no_arg_mnemonics.find(mnemonic);o!=no_arg_mnemonics.end()){
		emit_oneword(o->second);
		addr+=2;
	}
	else if(mnemonic=="DRW"){
		auto x = expect_register_or_symbol();
		expect_comma();
		auto y = expect_register_or_symbol();
		expect_comma();
		auto z = expect_number_or_label();
		limit_number(z,15);
		emit_twobytes(0xD0|x,(y<<4)|z);
		addr+=2;
	}
	else if(mnemonic=="RND"){
		auto x = expect_register_or_symbol();
		expect_comma();
		auto z = expect_number_or_label();
		limit_number(z,255);
		emit_twobytes(0xC0|x,z);
		addr+=2;
	}
	else if (mnemonic=="SYS"){
		auto a = expect_address();
		emit_oneword(0x0000|a);
		addr+=2;
	}
	else if (mnemonic=="CALL"){
		auto a = expect_address();
		emit_oneword(0x1000|a);
		addr+=2;
	}
	else if(mnemonic=="JP")
		process_jp();
	else if(mnemonic=="SNE")
		process_sne();
	else if(mnemonic=="SE")
		process_se();
	else if(mnemonic=="ADD")
		process_add();
	else if(mnemonic=="DB")	
		process_db();
	else if(mnemonic=="DW")
		process_dw();	
	else if (mnemonic=="LD"){
		process_ld();
		addr+=2;
	}
	//cout<<"mnemonic "<<mnemonic<<endl;
	auto tk = get_token();
	if(tk!=""){
		cout<<"Extraneous token "<<tk<<" on the line."<<endl;
		exit(0);
	}		
}

void check_dup_symbol(string tok)
{
	if((symtable.find(tok)!=symtable.end())||(regtable.find(tok)!=regtable.end())){
		cout<<"Label already taken"<<endl;
		exit(1);
	}
}

void process_line(string s)
{
	if(s.size()>0){
		curstr = s;
		pos = 0;
		is_eof = false;	
		auto tok1 = peek_token();
		if(is_reserved(tok1))			
			process_mnemonic();
		else {
			get_token();	
			if(!is_ident(tok1)){
				cout<<"Wrong name for a symbol"<<endl;
				exit(1);
			}
			check_dup_symbol(tok1);
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
				//cout<<"symbol "<<tok1<<" = "<<i<<endl;	
			}
			else {
				symtable[tok1] = addr;
				//cout<<"label "<<tok1<<" = "<<addr<<endl;	
				process_mnemonic();			
			}			
		}
	}	
}

void resolve_labels()
{
	for(auto &x: labelrefs)
	{
		auto it = symtable.find(x.second);
		if(it==symtable.end())
		{
			cout<<"Unresolved label reference: "<<x.second<<endl;
			exit(1);
		}else {			
			ss.clear();
			ss.seekg(x.first-0x200,ss.beg);
			char hi,lo;
			ss.get(hi);
			ss.get(lo);
			int hl = (hi<<8) | lo | it->second;
			ss.clear();
			ss.seekp(x.first-0x200,ss.beg);
			emit_twobytes(hl >> 8,hl%256);			
		}
	}
}

int main(int argc, char *argv[])
{
	cout<<"c8asm by shinkarom"<<endl;
	if(argc!=3)
	{
		cout<<"Both input file and output file required"<<endl;
		exit(1);
	}
	ifstream ifs(argv[1],ios::in);
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
	resolve_labels();
	ofstream ofs(argv[2],ios::binary|ios::out|ios::trunc);
	ofs<<ss.str();
	ofs.close();
	ss.clear();
	ss.seekg(0,ss.end);
	auto n = ss.tellg();
	cout<<argv[2]<<" assembled. "<<n<<" bytes."<<endl;
	return 0;
}