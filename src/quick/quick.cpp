using U=unsigned long long;using I=unsigned;using B=unsigned char;
extern "C" U quick_base();extern "C" void*quick_state();
template<class T>static T&at(void*p,U n){return*(T*)((B*)p+n);}template<class F>static F fn(U r){return(F)(quick_base()+r);}static void*ptr(U r){return(void*)(quick_base()+r);}
struct Quick{B disabled,initialized,playable,busy;B held[4];I swaps;void*unbuffed[4];};static Quick&q(){return at<Quick>(quick_state(),0x600);}static bool enabled(){return !q().disabled;}
extern "C" bool quick_get(void*p,int id){if(id==11)return enabled();return fn<bool(*)(void*,int)>(OLD_GET)(p,id);}
extern "C" void quick_set(void*p,int id,bool value){if(id!=11){fn<void(*)(void*,int,bool)>(OLD_SET)(p,id,value);return;}if(value==enabled())return;q().disabled=!value;q().playable=0;fn<bool(*)(void*)>(SAVE_OPTIONS)(ptr(OPTIONS));}
extern "C" void quick_load(void*f){fn<void(*)(void*)>(OLD_LOAD)(f);bool on=true;if(!fn<bool(*)(void*,const char*,bool*)>(READ_BOOL)(f,"TeamSwapQuickSwap",&on))on=true;q().disabled=!on;q().playable=0;}
extern "C" void quick_save(void*f){fn<void(*)(void*)>(OLD_SAVE)(f);fn<void(*)(void*,const char*,bool)>(WRITE_BOOL)(f,"TeamSwapQuickSwap",enabled());}
struct Entry{B bytes[0xe8];};static void copy(Entry&d,const Entry&s){for(I j=0;j<0xe8;j++)d.bytes[j]=s.bytes[j];}
extern "C" void quick_init(){fn<void(*)()>(OLD_INIT)();if(q().initialized||!at<B>(quick_state(),0x501))return;Entry*p=(Entry*)((B*)quick_state()+0x1000+10*0xe8);for(int j=7;j>=5;j--)copy(p[j+2],p[j]);copy(p[5],p[3]);copy(p[6],p[4]);const char*key="QuickSwapSet";at<const char*>(p+5,0)=key;at<I>(p+5,8)=fn<I(*)(const char*,int,int)>(HASH)(key,12,0);at<I>(p+5,12)=12;at<I>(p+6,0x18)=11;at<int>(p+1,0x68)=105;at<int>(p+2,0x68)=130;at<int>(p+3,0x68)=185;at<int>(p+4,0x68)=210;at<int>(p+5,0x68)=265;at<int>(p+6,0x68)=290;q().initialized=1;}
struct Key{const char*key;I hash;unsigned short length,reserved;};
static bool matches(const char*a,const char*b){if(!a)return false;I j=0;for(;a[j]&&a[j]==b[j];j++){}return a[j]==b[j];}
extern "C" const char*quick_string(void*t,const Key*k,int lang){if(k&&matches(k->key,"HideCharacterPortraitsSet"))return "Hide HUD";if(k&&k->key){const char*s="QuickSwapSet";I j=0;for(;s[j]&&s[j]==k->key[j];j++){}if(!s[j]&&!k->key[j])return "Quick Swap";}return fn<const char*(*)(void*,const Key*,int)>(OLD_STRING)(t,k,lang);}
static void*party(I slot){return fn<void*(*)(void*,int)>(GET_PLAYER)(ptr(GC),slot);}static bool live(void*p){return p&&!(at<I>(p,0x18)&0x80000001)&&at<B>(p,0x1d)<=1&&(at<I>(p,0x600)==3||(at<I>(p,0x1b0)&1))&&!at<B>(p,0x3a0)&&!at<B>(p,0x3a2)&&!at<B>(p,0x3a5)&&at<float>(p,0x2f4)>0&&(at<I>(p,0x600)!=0||!at<void*>(p,0x758));}
extern "C" void quick_lose_potions(void*,I);
extern "C" void quick_tick(){auto&v=q();void*im=ptr(INPUT);B down[4]={},edge[4]={};bool focus=at<I>(im,0x3848)==1;for(I h=0;h<4;h++){void*d=(B*)im+0x30f8+h*0xe8;if(focus&&at<B>(d,1)){down[h]=(at<B>(d,2+137)?1:0)|(at<B>(d,2+138)?2:0);}edge[h]=down[h]&~v.held[h];v.held[h]=down[h];if(focus&&at<B>(d,1)&&at<B>(d,0)==1){B wheel=(at<float>(d,0xd8)>.01f?1:0)|(at<float>(d,0xdc)>.01f?2:0);edge[h]|=wheel;down[h]|=wheel;}}
 I chapter=at<I>(ptr(GC),8);I count=at<I>(ptr(GC),0x54);bool playing=enabled()&&focus&&!v.busy&&at<I>(ptr(GC),0)==0&&!at<void*>(ptr(GC),0x160)&&!at<B>(ptr(WORLD),0x61d58)&&!fn<bool(*)()>(IS_PAUSED)()&&!fn<bool(*)(void*)>(ONLINE)(ptr(GC))&&chapter<=24&&((0x100e8eeu>>chapter)&1)&&count>=1&&count<=9;
 if(!playing){v.playable=0;return;}bool ready=v.playable;v.playable=1;
 // Reserve only the two raw pad buttons while Quick Swap is enabled in gameplay.
 for(I h=0;h<4;h++){void*d=(B*)im+0x30f8+h*0xe8;at<B>(d,2+137)=0;at<B>(d,2+138)=0;if(at<B>(d,0)==1){at<float>(d,0xd8)=at<float>(d,0xdc)=0;at<B>(d,2+121)=at<B>(d,2+122)=0;}}
 if(!ready||at<I>(ptr(DRAFT),0))return;
 const B ids[9]={0,8,1,2,4,6,9,3,5};I used=0;for(I j=0;j<count;j++){void*p=party(j);if(p&&at<I>(p,0x600)<32)used|=1u<<at<I>(p,0x600);}
 for(I slot=0;slot<count&&slot<4;slot++){void*p=party(slot);if(!live(p))continue;int h=at<int>(p,0x3e0);if(h<0||h>3||!edge[h]||down[h]==3)continue;int index=-1;for(int j=0;j<9;j++)if(ids[j]==at<I>(p,0x600))index=j;if(index<0)continue;int dir=(edge[h]&1)?-1:1;for(int j=0;j<9;j++){index=(index+dir+9)%9;if(!(used&(1u<<ids[index]))){at<I>(ptr(DRAFT),0)=(index+1)<<(slot*8);v.busy=1;fn<void(*)(void*,int)>(APPLY)(nullptr,0);v.busy=0;at<I>(ptr(DRAFT),0)=0;void*incoming=party(slot);if(incoming&&incoming!=p){quick_lose_potions(incoming,slot);++v.swaps;}return;}}}
}

// A retiring player can still own a live TargetProj. Use vanilla aim teardown
// after staging has succeeded, before removing that outgoing player.
extern "C" void swap_retire(void*p){fn<void(*)(void*)>(BREAK_LOCK)(p);fn<void(*)(void*)>(REMOVE_OBJECT)(p);}

extern "C" unsigned quick_damage(void*actor){B value=at<B>(ptr(DAMAGE_MULT),0);if(value>1)for(I j=0;j<4;j++)if(q().unbuffed[j]==actor)return 1;return value;}
extern "C" unsigned quick_damage_slot(I slot){return quick_damage(party(slot));}
extern "C" bool attack_pickup_original(void*,void*);
extern "C" bool quick_attack_pickup(void*power,void*collector){for(I j=0;j<4;j++)q().unbuffed[j]=nullptr;return attack_pickup_original(power,collector);}
static bool shield_potion(){void*f=at<void*>(ptr(GC),0x48);if(!f)return false;I n=at<I>(f,0x70);void**list=at<void**>(f,0x380);if(!list||n>10000)return false;for(I j=0;j<n;j++){void*p=list[j];if(p&&at<U>(p,0x10)==quick_base()+DEFENSE_MH&&!(at<I>(p,0x18)&0x80000000)&&at<B>(p,0x1d)==2&&at<float>(p,0x228)>0)return true;}return false;}
extern "C" void quick_lose_potions(void*p,I slot){bool attack=at<B>(ptr(DAMAGE_MULT),0)>1;bool shield=shield_potion();q().unbuffed[slot]=attack?p:nullptr;if(shield)at<I>(p,0x364)&=~0x8000u;if(!attack&&!shield)return;
 // The two vanilla potion auras share this handler. Retain across detach/delete.
 for(I j=0;j<8;j++){void*fx=fn<void*(*)(void*,void*,int)>(FIND_CHILD)(p,ptr(POTION_FX),0);if(!fx)break;++at<unsigned short>(fx,0x1e);fn<void(*)(void*)>(DETACH)(fx);fn<void(*)(void*)>(REMOVE_OBJECT)(fx);fn<void(*)(void*)>(RELEASE)(fx);}
}

// Hide only the numeric native personal-ammo element when its displayed value is zero.
extern "C" void pickup_hud_render(void*h,void*renderer,float alpha){void*part=at<void*>(h,0x70);void*sprite=at<void*>(h,0x78);void*owner=at<void*>(h,0xb0);if(part&&sprite&&owner&&at<U>(h,0x98)==quick_base()+HUD_CALLBACK&&at<I>(part,0)==2&&at<I>(sprite,44)==1){if(!(fn<float(*)(void*,int)>(HUD_CALLBACK)(owner,2)>0))return;}fn<void(*)(void*,void*,float)>(OLD_HUD_RENDER)(h,renderer,alpha);}
