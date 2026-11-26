using U=unsigned long long;using I=unsigned;using B=unsigned char;
extern "C" U ammo_base();extern "C" void* ammo_state();
template<class T>static T&at(void*p,U n){return *(T*)((B*)p+n);}template<class F>static F fn(U r){return(F)(ammo_base()+r);}
struct S{B apple[128];B cookie[128];void*clocks[8];I spawning;I spawned;I failures;};static S&s(){return*(S*)ammo_state();}
static void* file(B*cf,const char*path){if(!at<B>(cf,0)){at<const char*>(cf,8)=path;at<int>(cf,64)=at<int>(cf,68)=at<int>(cf,72)=-1;if(!fn<bool(*)(void*)>(LOAD)(cf))return nullptr;}return at<void*>(cf,40);}
static int root(void*f,int type){if(!f)return-1;I n=at<I>(f,112);void*p=at<void*>(f,16);if(!p||n>100000)return-1;for(I i=0;i<n;i++)if(at<unsigned short>(p,i*64+10)==type)return i;return-1;}
static void spawn(void*clock,void*f,int type,void*handler,float offset){int index=root(f,type);if(index<0){++s().failures;return;}void*p=fn<void*(*)(void*,void*,int,void*,I,unsigned short)>(CREATE)((void*)(ammo_base()+WORLD),f,index,handler,12,65535);if(!p){++s().failures;return;}
 for(I j=0;j<4;j++){float value=at<float>(clock,0xc0+j*4);if(j==2)value+=offset;at<float>(p,0xc0+j*4)=value;at<float>(p,0x1d0+j*4)=value;}
 at<float>(p,0x1e8)=at<float>(clock,0x1e8);at<B>(p,0x1f0)=at<B>(clock,0x1f0);fn<void*(*)(void*,void*)>(MOVE)((void*)(ammo_base()+TREE),p);++s().spawned;}
extern "C" U arena_clock(void*p,int msg,void*arg){U result=fn<U(*)(void*,int,void*)>(ORIGINAL)(p,msg,arg);auto&v=s();
 if(msg==4){for(I i=0;i<8;i++)if(v.clocks[i]==p)v.clocks[i]=nullptr;return result;}
 if(msg!=1||!p||v.spawning||*(I*)(ammo_base()+GC+8)!=24||at<void*>(p,0x80)!=*(void**)(ammo_base()+GAMEPLAY)||fn<bool(*)(void*)>(ONLINE)((void*)(ammo_base()+GC)))return result;
 for(I j=0;j<8;j++)if(v.clocks[j]==p)return result;I slot=0;for(;slot<8;slot++)if(!v.clocks[slot])break;if(slot==8)return result;v.clocks[slot]=p;v.spawning=1;
 // Load native source geometry during the map's clock initialization, rather
 // than during collection. Only the chosen pickup roots are instantiated.
 void*a=file(v.apple,"gamedata\\Levels\\Level03\\Gameplay");void*c=file(v.cookie,"gamedata\\Levels\\Level06\\Gameplay");
 spawn(p,a,139,(void*)(ammo_base()+APPLE),-.55f);spawn(p,c,140,(void*)(ammo_base()+COOKIE),.55f);v.spawning=0;return result;}
