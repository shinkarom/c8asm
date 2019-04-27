	const char decdigits[] = {'0','1','2','3','4','5','6','7','8','9'};
	const char decdigits_nonzero[] = {'1','2','3','4','5','6','7','8','9'};
	const char hexdigits[] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	const char octdigits[] = {'0','1','2','3','4','5','6','7'};
	const char whitespace[] = {' ','\t'};
	const vector<string> reserved = {"cls","ret","sys","jp","call","se","sne","ld","add","or","and","xor","sub","shr","subn","shl","sne","rnd","drw","skp","sknp","scd","scr","scl","exit","low","high","i","dt","st","f","b","v0","v1","v2","v3","v4","v5","v6","v7","v8","v9","va","vb","vc","vd","ve","vf","db","dw"}; 

struct Tokenizer
{
private:

public:
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
		while(is_whitespace(curstr[pos]))
			pos++;
		is_eof = pos==curstr.length();
		//cout<<"skipped whitespace from "<<old_pos<<" to "<<pos<<endl;
	}
	
	void expect_whitespace()
	{
		if(!is_whitespace(curstr[pos]))
		{
			cout<<"Whitespace expected."<<endl;
			exit(1);
		}
		skip_whitespace();
	}

	string peek_token()
	{
		auto k = get_token(true);
		//cout<<"peeked at token \""<<k<<"\""<<endl;
		pos = prev_pos;
		return k;
	}

	string get_token(bool do_peek = false)
	{
		skip_whitespace();
		prev_pos = pos;
		pos = curstr.find_first_of(" \t,",pos);
		if(pos==string::npos)
		{
			pos = curstr.length();
			is_eof = true;
		}
		auto g = curstr.substr(prev_pos,pos-prev_pos);	
		transform(g.begin(),g.end(),g.begin(),::tolower);
		
		//cout<<"got token \""<<g<<"\""<<endl;
		
		return g;
	}
	
	int expect_numberthing()
	{
		int result;
		auto tk = get_token();
		if(!is_number(tk)){
			if(auto it=symtable.find(tk);it!=symtable.end()){
				result = it->second;
			}else{
				cout<<"Numberthing expected, but got "<<tk<<endl;
				exit(0);
			}
		} else{		
			if(tk[0]=='%')
				result= convert_todec(tk.substr(1),2); else
			if(tk[0]=='$')
				result= convert_todec(tk.substr(1),16); else
			if(tk[0]=='0')
				result= convert_todec(tk.substr(1),8); else
			result= stoi(tk);
		}
		cout<<"got numberthing "<<result<<endl;
		return result;
	}
	
	int expect_register()
	{
		auto tk = peek_token();
		if(!is_register(tk))
		{
			cout<<"Register expected, but got "<<tk<<endl;
			exit(0);
		}
		else return get_register();
	}
	
	void expect_comma()
	{
		skip_whitespace();
		if(curstr[pos]!=',')
		{
			cout<<"Comma expected"<<endl;
			exit(0);
		}
		else {
			pos++;
		}
	}
	
	bool optional_comma()
	{
		int old_pos = pos;
		skip_whitespace();
		if(curstr[pos]==',')
		{
			pos++;
			return true;
		}
		pos=old_pos;
		return false;
	}
	
	int convert_todec(const string& input, int base)
	{
		if(base < 2 || base > 36)
			return 0;		
		bool isNegative = (input[0] == '-');    
		int startIndex = input.length()-1;
		int endIndex   = isNegative ? 1 : 0;		
		int value = 0;
		int digitValue = 1;		
		for(int i = startIndex; i >= endIndex; --i)
		{
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
		if(isNegative)
			value *= -1;		
		return value;
	}
	
	int get_register()
	{
		auto tk = get_token();
		auto n = convert_todec(tk.substr(1),16);
		return n;
	}
	
	bool is_number(string s)
	{
		if(is_decdigit_nonzero(s[0]))
		{
			for(size_t i = 1; i<s.length();i++)
			{
				if(!is_decdigit(s[i]))
				{
					return false;
				}
			}
			return true;
		} else
		if(s[0]=='$')
		{
			for(size_t i = 1; i<s.length();i++)
			{
				if(!is_hexdigit(s[i]))
				{
					return false;
				}
			}
			return true;
		} else	
		if(s[0]=='%')
		{
			for(size_t i = 1; i<s.length();i++)
			{
				if(!is_bindigit(s[i]))
				{
					return false;
				}
			}
			return true;
		} else		
		if(s[0]=='0')
		{
			for(size_t i = 1; i<s.length();i++)
			{
				if(!is_octdigit(s[i]))
				{
					return false;
				}
			}
			return true;
		} else
			return false;
	}
	
	bool is_register(string s)
	{
		return (s.length()==2)&&(s[0]=='v')&&(is_hexdigit(s[1]));
	}
	
	bool is_bindigit(char c)
	{
		return (c=='0')|(c=='1');	
	}
	
	bool is_octdigit(char c)
	{
		return find(begin(octdigits),end(octdigits),c)!=end(octdigits);		
	}
	
	bool is_hexdigit(char c)
	{
		return find(begin(hexdigits),end(hexdigits),c)!=end(hexdigits);		
	}
	
	bool is_decdigit_nonzero(char c)
	{
		return find(begin(decdigits_nonzero),end(decdigits_nonzero),c)!=end(decdigits_nonzero);		
	}
	
	bool is_decdigit(char c)
	{
		return find(begin(decdigits),end(decdigits),c)!=end(decdigits);		
	}
	
	bool is_whitespace(char c)
	{
		return find(begin(whitespace),end(whitespace),c)!=end(whitespace);
	}
	
	bool is_reserved(string s)
	{
		return find(begin(reserved),end(reserved),s)!=end(reserved);
	}
};