#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
using socket_t=SOCKET;
#define CLOSE closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using socket_t=int;
#define CLOSE close
#endif
using namespace std;

const int PORT=8080, HSIZE=101;
const string DB="properties.db";

string esc(const string&s){string o;for(char c:s){if(c=='"')o+="\\\"";else if(c=='\\')o+="\\\\";else if(c=='\n')o+="\\n";else o+=c;}return o;}
string lowerS(string s){transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return tolower(c);});return s;}
string jstr(const string&b,const string&k){string p="\""+k+"\"",x;size_t i=b.find(p);if(i==string::npos)return"";i=b.find(':',i+p.size());if(i==string::npos)return"";i++;while(i<b.size()&&isspace((unsigned char)b[i]))i++;if(i>=b.size()||b[i]!='"')return"";i++;bool e=false;for(;i<b.size();i++){char c=b[i];if(e){x+=c;e=false;}else if(c=='\\')e=true;else if(c=='"')break;else x+=c;}return x;}
double jnum(const string&b,const string&k,double d=0){string p="\""+k+"\"";size_t i=b.find(p);if(i==string::npos)return d;i=b.find(':',i+p.size());if(i==string::npos)return d;i++;while(i<b.size()&&isspace((unsigned char)b[i]))i++;size_t e=i;while(e<b.size()&&(isdigit((unsigned char)b[e])||b[e]=='.'||b[e]=='-'))e++;try{return stod(b.substr(i,e-i));}catch(...){return d;}}
bool jbool(const string&b,const string&k,bool d=true){string p="\""+k+"\"";size_t i=b.find(p);if(i==string::npos)return d;i=b.find(':',i+p.size());if(i==string::npos)return d;string x=lowerS(b.substr(i+1,8));return x.find("false")==string::npos;}
string qparam(const string&q,const string&k){string p=k+"=";size_t i=q.find(p);if(i==string::npos)return"";i+=p.size();size_t e=q.find('&',i);return q.substr(i,e==string::npos?string::npos:e-i);}

struct Property{string id,owner,location,type;double price=0,area=0;bool available=true;};
struct Request{int id;string customer,phone,propertyId,message,status;};
struct Booking{int id;string propertyId,customer,phone,date,status;};

// Linked List
struct PNode{Property p;PNode*next=nullptr;PNode(const Property&x):p(x){}};
class LinkedList{
 PNode*head=nullptr; int n=0;
public:
 ~LinkedList(){clear();}
 void clear(){while(head){auto*t=head;head=head->next;delete t;}n=0;}
 void add(const Property&p){auto*x=new PNode(p);if(!head)head=x;else{auto*c=head;while(c->next)c=c->next;c->next=x;}n++;}
 bool remove(const string&id){PNode*c=head,*pr=nullptr;while(c){if(c->p.id==id){if(pr)pr->next=c->next;else head=c->next;delete c;n--;return true;}pr=c;c=c->next;}return false;}
 Property* find(const string&id){for(auto*c=head;c;c=c->next)if(c->p.id==id)return &c->p;return nullptr;}
 vector<Property> all()const{vector<Property>v;for(auto*c=head;c;c=c->next)v.push_back(c->p);return v;}
};

// Queue
struct RNode{Request r;RNode*next=nullptr;RNode(const Request&x):r(x){}};
class Queue{
 RNode*f=nullptr,*r=nullptr;int n=0;
public:
 ~Queue(){clear();} void clear(){while(f){auto*t=f;f=f->next;delete t;}r=nullptr;n=0;}
 void push(const Request&x){auto*t=new RNode(x);if(!r)f=r=t;else r->next=t,r=t;n++;}
 bool pop(Request&x){if(!f)return false;auto*t=f;x=t->r;f=f->next;if(!f)r=nullptr;delete t;n--;return true;}
 vector<Request>all()const{vector<Request>v;for(auto*c=f;c;c=c->next)v.push_back(c->r);return v;}
};

// Stack
struct BNode{Booking b;BNode*next=nullptr;BNode(const Booking&x):b(x){}};
class Stack{
 BNode*top=nullptr;int n=0;
public:
 ~Stack(){clear();} void clear(){while(top){auto*t=top;top=top->next;delete t;}n=0;}
 void push(const Booking&x){auto*t=new BNode(x);t->next=top;top=t;n++;}
 bool pop(Booking&x){if(!top)return false;auto*t=top;x=t->b;top=top->next;delete t;n--;return true;}
 vector<Booking>all()const{vector<Booking>v;for(auto*c=top;c;c=c->next)v.push_back(c->b);return v;}
};

// Hash table with separate chaining
struct HNode{string key;Property*value;HNode*next;HNode(string k,Property*v):key(k),value(v){}};
class HashTable{
 HNode*t[HSIZE]{};
 int hash(const string&s)const{unsigned long h=5381;for(unsigned char c:s)h=((h<<5)+h)+c;return h%HSIZE;}
public:
 ~HashTable(){clear();}
 void clear(){for(auto&i:t){while(i){auto*x=i;i=i->next;delete x;}}}
 void put(Property*p){int h=hash(p->id);for(auto*x=t[h];x;x=x->next)if(x->key==p->id){x->value=p;return;}auto*x=new HNode(p->id,p);x->next=t[h];t[h]=x;}
 Property*get(const string&k)const{for(auto*x=t[hash(k)];x;x=x->next)if(x->key==k)return x->value;return nullptr;}
 bool del(const string&k){int h=hash(k);HNode*x=t[h],*pr=nullptr;while(x){if(x->key==k){if(pr)pr->next=x->next;else t[h]=x->next;delete x;return true;}pr=x;x=x->next;}return false;}
};

string pjson(const Property&p){ostringstream o;o<<"{\"id\":\""<<esc(p.id)<<"\",\"owner\":\""<<esc(p.owner)<<"\",\"location\":\""<<esc(p.location)<<"\",\"type\":\""<<esc(p.type)<<"\",\"price\":"<<p.price<<",\"area\":"<<p.area<<",\"available\":"<<(p.available?"true":"false")<<"}";return o.str();}
string rjson(const Request&r){ostringstream o;o<<"{\"requestId\":"<<r.id<<",\"customer\":\""<<esc(r.customer)<<"\",\"phone\":\""<<esc(r.phone)<<"\",\"propertyId\":\""<<esc(r.propertyId)<<"\",\"message\":\""<<esc(r.message)<<"\",\"status\":\""<<esc(r.status)<<"\"}";return o.str();}
string bjson(const Booking&b){ostringstream o;o<<"{\"bookingId\":"<<b.id<<",\"propertyId\":\""<<esc(b.propertyId)<<"\",\"customer\":\""<<esc(b.customer)<<"\",\"phone\":\""<<esc(b.phone)<<"\",\"date\":\""<<esc(b.date)<<"\",\"status\":\""<<esc(b.status)<<"\"}";return o.str();}

template<class T,class F> string arr(const vector<T>&v,F fn){ostringstream o;o<<"[";for(size_t i=0;i<v.size();i++){if(i)o<<",";o<<fn(v[i]);}o<<"]";return o.str();}

class App{
public:
 LinkedList props;HashTable table;Queue requests;Stack bookings;int nextReq=1001,nextBook=5001;
 void rebuild(){table.clear();for(auto&p:props.all())if(auto*x=props.find(p.id))table.put(x);}
 void save(){ofstream f(DB);for(auto&p:props.all())f<<p.id<<'|'<<p.owner<<'|'<<p.location<<'|'<<p.type<<'|'<<p.price<<'|'<<p.area<<'|'<<(p.available?1:0)<<'\n';}
 void load(){ifstream f(DB);string l;props.clear();while(getline(f,l)){stringstream s(l);vector<string>x;string a;while(getline(s,a,'|'))x.push_back(a);if(x.size()!=7)continue;Property p{x[0],x[1],x[2],x[3],stod(x[4]),stod(x[5]),x[6]=="1"};props.add(p);}rebuild();}
 void init(){ifstream f(DB);if(f.good()){load();if(!props.all().empty())return;}props.add({"RE101","Aarav Sharma","Bangalore","Apartment",6500000,1200,true});props.add({"RE102","Meera Rao","Mysore","Villa",12000000,2500,true});props.add({"RE103","Vikram Nair","Bangalore","Plot",4000000,1500,true});props.add({"RE104","Ananya Iyer","Chennai","Apartment",5800000,1100,false});props.add({"RE105","Rohan Kapoor","Hyderabad","Villa",15000000,3000,true});rebuild();save();}
 string allProps(){return arr(props.all(),[](const Property&p){return pjson(p);});}
 string allReq(){return arr(requests.all(),[](const Request&r){return rjson(r);});}
 string allBook(){return arr(bookings.all(),[](const Booking&b){return bjson(b);});}
 string addProp(const string&b){Property p{jstr(b,"id"),jstr(b,"owner"),jstr(b,"location"),jstr(b,"type"),jnum(b,"price"),jnum(b,"area"),jbool(b,"available")};if(p.id.empty()||p.location.empty()||p.type.empty())return R"({"success":false,"message":"ID, location and type are required."})";if(table.get(p.id))return R"({"success":false,"message":"Property ID already exists."})";props.add(p);table.put(props.find(p.id));save();return R"({"success":true,"message":"Property added successfully."})";}
 string updateProp(string id,const string&b){auto*p=table.get(id);if(!p)return R"({"success":false,"message":"Property not found."})";string x;if(!(x=jstr(b,"owner")).empty())p->owner=x;if(!(x=jstr(b,"location")).empty())p->location=x;if(!(x=jstr(b,"type")).empty())p->type=x;if(b.find("\"price\"")!=string::npos)p->price=jnum(b,"price",p->price);if(b.find("\"area\"")!=string::npos)p->area=jnum(b,"area",p->area);if(b.find("\"available\"")!=string::npos)p->available=jbool(b,"available",p->available);save();return R"({"success":true,"message":"Property updated successfully."})";}
 string delProp(string id){if(!table.get(id))return R"({"success":false,"message":"Property not found."})";table.del(id);props.remove(id);save();return R"({"success":true,"message":"Property deleted successfully."})";}
 string search(string id){auto*p=table.get(id);if(!p)return R"({"success":false,"message":"Property not found."})";return "{\"success\":true,\"property\":"+pjson(*p)+"}";}
 string addReq(const string&b){Request r{nextReq++,jstr(b,"customer"),jstr(b,"phone"),jstr(b,"propertyId"),jstr(b,"message"),"Waiting"};if(r.customer.empty()||r.propertyId.empty())return R"({"success":false,"message":"Customer and property ID are required."})";requests.push(r);return "{\"success\":true,\"message\":\"Request added to queue.\",\"request\":"+rjson(r)+"}";}
 string processReq(){Request r;if(!requests.pop(r))return R"({"success":false,"message":"Queue is empty."})";r.status="Processed";return "{\"success\":true,\"message\":\"Oldest request processed.\",\"request\":"+rjson(r)+"}";}
 string addBook(const string&b){Booking x{nextBook++,jstr(b,"propertyId"),jstr(b,"customer"),jstr(b,"phone"),jstr(b,"date"),"Confirmed"};auto*p=table.get(x.propertyId);if(!p)return R"({"success":false,"message":"Property not found."})";if(!p->available)return R"({"success":false,"message":"Property is already booked."})";if(x.customer.empty())return R"({"success":false,"message":"Customer name is required."})";p->available=false;bookings.push(x);save();return "{\"success\":true,\"message\":\"Property booked successfully.\",\"booking\":"+bjson(x)+"}";}
 string cancel(){Booking x;if(!bookings.pop(x))return R"({"success":false,"message":"Booking stack is empty."})";if(auto*p=table.get(x.propertyId))p->available=true;save();return R"({"success":true,"message":"Latest booking cancelled and property released."})";}
};

string httpResp(string body,int code=200,string text="OK"){ostringstream o;o<<"HTTP/1.1 "<<code<<" "<<text<<"\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS\r\nAccess-Control-Allow-Headers: Content-Type\r\nContent-Length: "<<body.size()<<"\r\nConnection: close\r\n\r\n"<<body;return o.str();}
void sendAll(socket_t s,const string&x){size_t n=0;while(n<x.size()){int z=send(s,x.data()+n,(int)(x.size()-n),0);if(z<=0)break;n+=z;}}
void client(socket_t s,App&a){char buf[65536]{};int n=recv(s,buf,sizeof(buf)-1,0);if(n<=0){CLOSE(s);return;}string req(buf,n);size_t h=req.find("\r\n\r\n"); string body=h==string::npos?"":req.substr(h+4);istringstream q(req.substr(0,h));string method,target,ver;q>>method>>target>>ver;string path=target,query;size_t z=target.find('?');if(z!=string::npos){path=target.substr(0,z);query=target.substr(z+1);}string out;int code=200;string st="OK";
 if(method=="OPTIONS")out="{}";
 else if(path=="/api/health")out=R"({"success":true,"message":"Backend is running"})";
 else if(path=="/api/properties"&&method=="GET")out=a.allProps();
 else if(path=="/api/properties"&&method=="POST")out=a.addProp(body);
 else if(path=="/api/requests"&&method=="GET")out=a.allReq();
 else if(path=="/api/requests"&&method=="POST")out=a.addReq(body);
 else if(path=="/api/requests/process"&&method=="POST")out=a.processReq();
 else if(path=="/api/bookings"&&method=="GET")out=a.allBook();
 else if(path=="/api/bookings"&&method=="POST")out=a.addBook(body);
 else if(path=="/api/bookings/cancel-latest"&&method=="POST")out=a.cancel();
 else if(path=="/api/search"&&method=="GET")out=a.search(qparam(query,"id"));
 else if(path.rfind("/api/properties/",0)==0){string id=path.substr(16);if(method=="GET")out=a.search(id);else if(method=="PUT")out=a.updateProp(id,body);else if(method=="DELETE")out=a.delProp(id);else{out=R"({"success":false,"message":"Method not allowed"})";code=405;st="Method Not Allowed";}}
 else{out=R"({"success":false,"message":"Endpoint not found"})";code=404;st="Not Found";}
 sendAll(s,httpResp(out,code,st));CLOSE(s);}

int main(){
#ifdef _WIN32
 WSADATA w; if(WSAStartup(MAKEWORD(2,2),&w)!=0)return 1;
#endif
 App app;app.init();socket_t srv=socket(AF_INET,SOCK_STREAM,0);if(srv<0){cerr<<"Socket failed\n";return 1;}int opt=1;setsockopt(srv,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=INADDR_ANY;addr.sin_port=htons(PORT);if(bind(srv,(sockaddr*)&addr,sizeof(addr))<0){cerr<<"Port "<<PORT<<" unavailable\n";return 1;}if(listen(srv,10)<0)return 1;cout<<"Real Estate backend running at http://localhost:8080\n";cout<<"DSA: Linked List | Queue | Stack | Hash Table\n";while(true){sockaddr_in c{};
#ifdef _WIN32
 int l=sizeof(c);
#else
 socklen_t l=sizeof(c);
#endif
 socket_t s=accept(srv,(sockaddr*)&c,&l);if(s>=0)client(s,app);}CLOSE(srv);
#ifdef _WIN32
 WSACleanup();
#endif
}
