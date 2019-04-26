#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
using namespace std;

vector<string> mnemonics = {"cls","ret","sys","sys","jp","call","se","sne","ld","add","or","and","xor","sub","shr","subn","shl","sne","rnd","drw","skp","sknp","scd","scr","scl","exit","low","high"}; 

vector<string> lines;

unordered_map<string, int> symtable;
unordered_map<int, string> labelrefs;

struct Tokenizer
{
	string curstr;
	size_t pos;
	size_t prev_pos;
	bool is_eof;
	
	Tokenizer(string& arg)
	{
		curstr = arg;
		pos = 0;
		is_eof = false;
	}
	
	void skip_whitespace()
	{
		//auto old_pos = pos;
		char delims[] = {' ','\t'};
		while(find(begin(delims),end(delims),curstr[pos])!=end(delims))
			pos++;
		is_eof = pos==curstr.length();
		//cout<<"skipped whitespace from "<<old_pos<<" to "<<pos<<endl;
	}

	string get_token(bool do_peek = false)
	{
		skip_whitespace();
		prev_pos = pos;
		pos = curstr.find_first_of(" \t",pos);
		if(pos==string::npos)
		{
			pos = curstr.length();
			is_eof = true;
		}
		auto g = curstr.substr(prev_pos,pos-prev_pos);	
		transform(g.begin(),g.end(),g.begin(),::tolower);
		
		//if(do_peek)cout<<"peeked at token \"";
		//else cout<<"got token \"";
		//cout<<g<<"\""<<endl;
		
		skip_whitespace();
		if(do_peek) pos = prev_pos;
		return g;
	}
};

struct Instruction
{
	string mnemonic;
	vector<int> args;
	string labelref;
};

vector<Instruction> instructions;

string preprocess_line(string& arg)
{
	return arg.substr(0,arg.find(";",0));
}

void pass_1()
{
	int addr = 0x200;
	for(auto l: lines)
	{
		Tokenizer t(l);
		auto tok1 = t.get_token(true);
		//cout<<"tok1 is "<<tok1<<endl;
		if(find(begin(mnemonics),end(mnemonics),tok1)!=end(mnemonics))			
		{ 
			tok1 = t.get_token(false);
			cout<<"mnemonic "<<tok1<<endl;
			addr+=2;
		}
		else 	
		{
			t.get_token(false);	
			if(auto tok2 = t.get_token(true); tok2=="equ")
			{		
				t.get_token(false);
				auto tok3 = t.get_token(false);
				//cout<<"tok3 is "<<tok3<<endl;
				int i = stoi(tok3);
				symtable[tok1] = i;
				cout<<"added symbol "<<tok1<<" = "<<i<<endl;	
			}
			else
			{
				symtable[tok1] = addr;
				cout<<"added label "<<tok1<<" = "<<addr<<endl;			
			}			
		}
	}
}

void pass_2()
{
	for(auto I: instructions)
	{
		
	}
}

int main(int argc, char *argv[])
{
	ifstream ifs("test.asm",ios::in);
	char templine[256];
	while(ifs.getline(templine,256))
	{
		string thisline(templine);
		thisline = preprocess_line(thisline);
		if(thisline.size()>0)
		{
			lines.push_back(thisline);
		}
	}
	pass_1();
	pass_2();
	ofstream ofs("test.c8",ios::out|ios::trunc);
	return 0;
}