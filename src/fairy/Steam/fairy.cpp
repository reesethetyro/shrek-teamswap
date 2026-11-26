using U=unsigned long long;using I=unsigned;using B=unsigned char;
extern "C" U fairy_base();extern "C" void* fairy_state();
template<class T>static T& at(void*p,U n){return *(T*)((B*)p+n);}
template<class F>static F fn(U r){return (F)(fairy_base()+r);}
struct State{void*actor;void*fx;float remaining;I prior;I casts;I transfers;I expirations;I fx_failures;float sparkle_next;float full_duration;};
static State& s(){return *(State*)fairy_state();}
static void retain(void*p){if(p)++at<unsigned short>(p,0x1e);}
static void release(void*p){if(p)fn<void(*)(void*)>(0x2ad6d0)(p);}
static void* party(I i){return fn<void*(*)(void*,int)>(0x22c8e0)((void*)(fairy_base()+0x1d116f0),i);}
static bool active(void*p){I n=*(I*)(fairy_base()+0x1d11744);if(n>4)n=4;for(I i=0;i<n;i++)if(party(i)==p)return true;return false;}
static bool alive(void*p){return p&&!(at<I>(p,0x18)&0x80000001)&&at<B>(p,0x1d)!=14&&at<float>(p,0x2f4)>0;}
static void* defense(){
 void*f=*(void**)(fairy_base()+0x1d11738);if(!f)return nullptr;
 I n=at<I>(f,0x70);void**v=at<void**>(f,0x380);if(!v||n>10000)return nullptr;
 for(I i=0;i<n;i++){void*p=v[i];if(p&&at<U>(p,0x10)==fairy_base()+0x1e84b0&&!(at<I>(p,0x18)&0x80000000))return p;}return nullptr;
}
static void* fallback_file(){
 void*cf=(B*)fairy_state()+0x100;
 if(!at<B>(cf,0)){
  at<const char*>(cf,8)="gamedata\\Levels\\Level23\\Gameplay";
  at<int>(cf,0x40)=-1;at<int>(cf,0x44)=-1;at<int>(cf,0x48)=-1;
  if(!fn<bool(*)(void*)>(0x8ebe0)(cf))return nullptr;
 }
 void*f=at<void*>(cf,0x28);if(!f||at<I>(f,0x70)<=194)return nullptr;
 void*nodes=at<void*>(f,0x10);
 if(!nodes||at<unsigned short>(nodes,47*64+10)!=146)return nullptr;
 return f;
}
static bool potion(){void*p=defense();return p&&at<B>(p,0x1d)==2&&at<float>(p,0x228)>0;}
static void remove_fx(void*p){
 if(!p)return;
 if(!(at<I>(p,0x18)&0x80000000)){
  fn<void(*)(void*)>(0x2adbe0)(p);
  auto mh=at<U(*)(void*,int,void*)>(p,0x10);if(mh)mh(p,4,nullptr);
 }
 release(p);
}
extern "C" void fairy_reset(){
 auto &v=s();void*a=v.actor;void*f=v.fx;v.actor=nullptr;v.fx=nullptr;v.remaining=0;
 bool blue=potion();
 if(a&&!v.prior&&!blue&&at<B>(a,0x1d)!=14)at<I>(a,0x364)&=~0x8000u;
 if(f){if(blue)release(f);else remove_fx(f);}release(a);v.prior=0;
}
static void yield_to_potion(){
 // Remove only Fairy's owned aura before the native pickup scans the team.
 // Native potion FX setup aborts on any existing aura of this same type.
 void*f=s().fx;s().fx=nullptr;if(f)remove_fx(f);
 fairy_reset();
}
extern "C" bool fairy_potion_original(void*,void*);
extern "C" bool fairy_potion_pickup(void*power,void*collector){
 yield_to_potion();
 return fairy_potion_original(power,collector);
}
static void attach_fx(void*a){
 auto &v=s();
 // Exactly the native potion FX: original prefab reference, shared handler,
 // World::CreateObject flags, and Object::Attach. No replacement art.
 void*d=defense();
 void*existing=fn<void*(*)(void*,void*,int)>(0x8ce90)(a,(void*)(fairy_base()+0x1e83f0),0);
 if(existing)return;
 void*params=d?at<void*>(d,0x220):nullptr;void*file=d?at<void*>(d,0x80):nullptr;int root=params?at<int>(params,0):-1;
 if(!file||root<0){file=fallback_file();root=190;}if(!file){++v.fx_failures;return;}
 void*f=fn<void*(*)(void*,void*,int,void*,int,unsigned short)>(0x241dd0)((void*)(fairy_base()+0x1d13be0),file,root,(void*)(fairy_base()+0x1e83f0),12,65535);
 if(!f){++v.fx_failures;return;}
 retain(f);v.fx=f;fn<void(*)(void*,void*)>(0x2ad8e0)(a,f);
}
// Reuse each character's original levitation dust emitter and its native cadence.
// Temporarily supply only its two visual clocks; restore gameplay float state.
static void sparkles(void*a){
 auto &v=s();I timer=at<I>(a,0x59c),next=at<I>(a,0x654),total=at<I>(a,0x5a0);
 at<float>(a,0x5a0)=v.full_duration;at<float>(a,0x59c)=v.remaining;at<float>(a,0x654)=v.sparkle_next;
 auto emit=at<void(*)(void*)>(at<void*>(a,0),0xc8);if(emit)emit(a);
 v.sparkle_next=at<float>(a,0x654);
 at<I>(a,0x5a0)=total;at<I>(a,0x59c)=timer;at<I>(a,0x654)=next;
}
static bool shield_enabled(){return !at<B>(fairy_state(),0x500);}
extern "C" U fairy_hit(void*a,U,void*packet){
 if(shield_enabled()&&active(a)&&at<I>(a,0x600)!=3){
  if(!alive(a))return (U)-1;
  if(potion()){yield_to_potion();return 1;}
  // Some campaign chapters disable physical levitation with a zero duration.
  // Shield eligibility is independent of that map restriction. The ordinary
  // native levitation duration is 600 time units (10 seconds at normal speed).
  float duration=at<float>(a,0x5a0);if(!(duration>0&&duration<100000))duration=600.f;
  auto &v=s();
  if(v.actor==a){v.remaining=duration;v.sparkle_next=duration;v.full_duration=duration;at<I>(a,0x364)|=0x8000;++v.casts;return 1;}
  bool transfer=v.actor!=nullptr;fairy_reset();retain(a);v.actor=a;v.prior=(at<I>(a,0x364)&0x8000)!=0&&!potion();v.remaining=duration;v.sparkle_next=duration;v.full_duration=duration;
  at<I>(a,0x364)|=0x8000;++v.casts;if(transfer)++v.transfers;attach_fx(a);return 1;
 }
 // Preserve collision responses for enemies, scenery and Fairy herself.
 auto mh=at<U(*)(void*,int,void*)>(a,0x10);return mh?mh(a,11,packet):0;
}
extern "C" void fairy_lever_maintain(void*);
extern "C" void fairy_hud_attach(void*);
extern "C" void fairy_exit_contacts(void*);
extern "C" void fairy_tick(void*p){
 fairy_hud_attach(p);
 fairy_exit_contacts(p);
 fairy_lever_maintain(p);
 auto&v=s();if(!v.actor)return;
 if(potion()){yield_to_potion();return;}
 if(!active(v.actor)||!alive(v.actor)){fairy_reset();return;}
 if(p!=v.actor)return;
 // Same virtual timestep as Player::HandleFloating (pause/slow motion aware).
 auto dt=at<float(*)(void*)>(at<void*>(p,0),0x30);float d=dt(p);
 if(d>0&&d<100000)v.remaining-=d;
 if(!(v.remaining>0)){++v.expirations;fairy_reset();return;}
 at<I>(p,0x364)|=0x8000;
 // Potion expiration can remove the shared native FX while Fairy's spell lives.
 if(v.fx&&(at<I>(v.fx,0x18)&0x80000000)){release(v.fx);v.fx=nullptr;}
 if(!v.fx)attach_fx(p);
 if(d>0&&d<100000)sparkles(p);
}


// Fairy project: native lever interaction with a private cast-animation table.
struct alignas(16) Vec {float x,y,z,w;};
struct Trail {void*lever;float age,next;Vec from,to;float flight;};
struct LeverState {void*actor;void*actions;int cast[4];Trail trails[4];I offers,starts,particles;};
static LeverState& ls(){return *(LeverState*)((B*)fairy_state()+0x200);}
static bool bridge(void*p){return p&&(at<U>(p,0x10)==fairy_base()+0x1f4250||at<U>(p,0x10)==fairy_base()+0x1f6210)&&!(at<I>(p,0x18)&0x80000001);}
static bool fairy(void*p){return p&&at<I>(p,0x600)==3&&active(p)&&alive(p);}
static void restore_actions(){auto&v=ls();void*a=v.actor;if(a&&at<void*>(a,0x4f8)==v.cast)at<void*>(a,0x4f8)=v.actions;v.actor=nullptr;v.actions=nullptr;release(a);}
static void prepare_actions(void*a){
 auto&v=ls();if(v.actor==a)return;if(v.actor)restore_actions();
 retain(a);v.actor=a;v.actions=at<void*>(a,0x4f8);v.cast[0]=10;v.cast[1]=v.cast[2]=v.cast[3]=-1;at<void*>(a,0x4f8)=v.cast;
}
static bool waiting_for_lever(void*p){
 B mode=at<B>(p,0x1d);void*pending=at<void*>(p,0x660);
 return (mode==0||mode==1)&&bridge(pending)&&at<I>(pending,0x1d4)==0&&at<I>(p,0x680)==0;
}
extern "C" void fairy_lever_maintain(void*p){
 auto&v=ls();
 if(v.actor){void*a=v.actor;bool running=at<B>(a,0x1d)==8&&bridge(at<void*>(a,0x690));if(!fairy(a)||(!running&&!waiting_for_lever(a)))restore_actions();}
 if(fairy(p)&&waiting_for_lever(p)){at<Vec>(p,0x670)=at<Vec>(p,0xc0);prepare_actions(p);}
}
extern "C" void fairy_lever_reset(){auto&v=ls();restore_actions();for(I i=0;i<4;i++){void*p=v.trails[i].lever;v.trails[i].lever=nullptr;release(p);}}
static Vec world_position(void*p){Vec v{};fn<void*(*)(void*,Vec*)>(0x2aeac0)(p,&v);return v;}
extern "C" void fairy_lever_started(void*lever,void*actor){
 if(!fairy(actor)||!bridge(lever)||at<I>(lever,0x1d4)!=1)return;
 auto&v=ls();for(I i=0;i<4;i++)if(v.trails[i].lever==lever)return;
 for(I i=0;i<4;i++)if(!v.trails[i].lever){retain(lever);auto&t=v.trails[i];t={};t.lever=lever;
 void*wand=fn<void*(*)(void*,int)>(0xb9e90)(actor,0);
 t.from=world_position(wand?wand:actor);t.to=world_position(lever);t.to.y-=.04f;
 ++v.starts;return;}
}
// The rotating crystal is a child of the animated handle. Its origin marks
// the knob; move 1/8 unit toward the cap in HANDLE space, before world transforms.
static void* lever_knob(void*p,int depth=0){
 if(!p||depth>8)return nullptr;
 if(at<U>(p,0x10)==fairy_base()+0xb9aa0)return p;
 void*c=at<void*>(p,0x30);for(int n=0;c&&n<64;n++,c=at<void*>(c,0x28)){void*k=lever_knob(c,depth+1);if(k)return k;}return nullptr;
}
static bool tip_position(void*lever,Vec&v){
 void*k=lever_knob(lever);if(!k)return false;
 v=at<Vec>(k,0xc0);v.y-=.125f;
 void*p=at<void*>(k,0x38);int n=0;
 for(;p&&n<32;n++,p=at<void*>(p,0x38)){
  Vec x=at<Vec>(p,0x90),y=at<Vec>(p,0xa0),z=at<Vec>(p,0xb0),t=at<Vec>(p,0xc0);
  v={v.x*x.x+v.y*y.x+v.z*z.x+t.x,v.x*x.y+v.y*y.y+v.z*z.y+t.y,v.x*x.z+v.y*y.z+v.z*z.z+t.z,1};
 }
 return !p;
}
static bool pull_finished(void*lever){
 // Native clip length and native 8.8 animation clock; used-state remains 1 forever.
 void*a=at<void*>(lever,0x70);if(!a)return true;
 I frames=at<unsigned short>(a,0x12)&0x7fff;
 return !frames||at<U>(lever,0x1e8)>=(U)frames*256;
}
// Reuse the native shot's visual emitter only; no gameplay collision packet.
// The native function reads solely the emitter position at +0xc0.
static void shot_visual(const Vec&pos,const Vec&vel){
 struct alignas(16) Visual {B pad[0xc0];Vec pos;};Visual v;v.pos=pos;
 fn<void(*)(void*,const Vec&)>(0x103370)(&v,vel);
}
static bool shot_step(Trail&t,float d){
 if(t.flight>=18)return true;
 float prev=t.flight;t.flight+=d;if(t.flight>18)t.flight=18;
 Vec velocity{(t.to.x-t.from.x)/18,(t.to.y-t.from.y)/18,(t.to.z-t.from.z)/18,0};
 for(int j=1;j<=6;j++){
  float u=(prev+(t.flight-prev)*(float)j/6)/18;
  Vec p{t.from.x+(t.to.x-t.from.x)*u,t.from.y+(t.to.y-t.from.y)*u,t.from.z+(t.to.z-t.from.z)*u,1};
  shot_visual(p,velocity);
 }
 if(t.flight<18)return false;
 Vec still{0,-.003f,0,0};fn<void*(*)(const Vec&,const Vec&)>(0x101980)(t.to,still);
 return true;
}
extern "C" void fairy_lever_update(void*lever){
 auto&v=ls();
 for(I i=0;i<4;i++)if(v.trails[i].lever==lever){
  auto&t=v.trails[i];
  if(!bridge(lever)||at<I>(lever,0x1d4)!=1||pull_finished(lever)||t.age>1800){t.lever=nullptr;release(lever);continue;}
  float d=at<float(*)(void*)>(at<void*>(lever,0),0x30)(lever);if(!(d>0&&d<1000))continue;t.age+=d;
  if(!shot_step(t,d))continue;
  if(t.age>=t.next){t.next=t.age+2;
   Vec pos;if(!tip_position(lever,pos))continue;Vec vel{0,-.003f,0,0};
   fn<void*(*)(const Vec&,const Vec&)>(0x101980)(pos,vel);++v.particles;
  }
 }
}

// Native BoolOption 9 is private to Team Swap; stock uses 0..8.
extern "C" bool settings_get(){return shield_enabled();}
extern "C" bool settings_hide_get(){return at<B>(fairy_state(),0x502)!=0;}
extern "C" void settings_hide_set(void*,int,bool enabled){
 if(enabled==settings_hide_get())return;
 at<B>(fairy_state(),0x502)=enabled;
 fn<bool(*)(void*)>(0x1bdb30)((void*)(fairy_base()+0x5c36a0));
}
extern "C" void settings_set(void*,int,bool enabled){
 bool changed=enabled!=shield_enabled();at<B>(fairy_state(),0x500)=!enabled;
 if(!enabled)fairy_reset();
 if(changed)fn<bool(*)(void*)>(0x1bdb30)((void*)(fairy_base()+0x5c36a0));
}
extern "C" void settings_load(void*f){
 bool enabled=true;if(!fn<bool(*)(void*,const char*,bool*)>(0x1bded0)(f,"TeamSwapFairyShield",&enabled))enabled=true;
 at<B>(fairy_state(),0x500)=!enabled;
 bool hidden=false;if(!fn<bool(*)(void*,const char*,bool*)>(0x1bded0)(f,"TeamSwapHidePortraits",&hidden))hidden=false;
 at<B>(fairy_state(),0x502)=hidden;
}
extern "C" void settings_save(void*f){
 fn<void(*)(void*,const char*,bool)>(0x1be2f0)(f,"TeamSwapFairyShield",shield_enabled());
 fn<void(*)(void*,const char*,bool)>(0x1be2f0)(f,"TeamSwapHidePortraits",settings_hide_get());
}
struct Entry {B bytes[0xe8];};
static Entry* team_page(){return (Entry*)((B*)fairy_state()+0x1000);}
static Entry* settings_page(){return team_page()+10;}
static void copy_entry(Entry&d,const Entry&s){for(I i=0;i<0xe8;i++)d.bytes[i]=s.bytes[i];}
static void text(Entry&e,const char*s,I hash,I len){at<const char*>(&e,0)=s;at<I>(&e,8)=hash;at<I>(&e,12)=len;}
extern "C" void settings_init(){
 if(at<B>(fairy_state(),0x501))return;
 Entry*source=*(Entry**)(fairy_base()+0x5b8370+133*8);if(!source||at<I>(source,0x10)!=0)return;
 Entry*d=(Entry*)(fairy_base()+0x1d0dc50);if(at<I>(d+4,0x20)!=8)return;
 Entry*t=team_page();for(I i=0;i<8;i++)copy_entry(t[i+(i>=2)],source[i]);
 copy_entry(t[2],source[1]);text(t[2],"TeamSwapSettings",74734751,16);at<I>(t+2,0x18)=109;
 for(I i=2;i<=5;i++)if(at<int>(t+i,0x68)>0)at<int>(t+i,0x68)+=30;
 // Source Swap/Char/Upgrades/Bonus use explicit 30-pixel row spacing.
 at<int>(t+2,0x68)=at<int>(t+1,0x68)+30;
 Entry*p=settings_page();copy_entry(p[0],d[0]);text(p[0],"TeamSwapSettings",74734751,16);at<int>(p,0x58)=60;
 copy_entry(p[1],d[3]);text(p[1],"FairyShieldSpecialSet",1235039184,20);at<int>(p+1,0x68)=125;
 copy_entry(p[2],d[4]);at<I>(p+2,0x18)=9;at<int>(p+2,0x68)=150;
 copy_entry(p[3],d[3]);text(p[3],"HideCharacterPortraitsSet",329245417,25);at<int>(p+3,0x68)=210;
 copy_entry(p[4],d[4]);at<I>(p+4,0x18)=10;at<int>(p+4,0x68)=235;
 copy_entry(p[5],source[5]);at<int>(p+5,0x58)=-1000001;at<int>(p+5,0x68)=360;
 copy_entry(p[6],source[6]);copy_entry(p[7],d[7]);
 *(Entry**)(fairy_base()+0x5b8370+133*8)=t;
 *(Entry**)(fairy_base()+0x5b8370+109*8)=p;
 at<B>(fairy_state(),0x501)=1;
}

extern "C" float fairy_countdown_value(void*p,int){
 auto&v=s();if(potion()||!shield_enabled()||!p||at<I>(p,0x600)!=3||!active(p)||!v.actor||!active(v.actor)||!alive(v.actor)||!(v.remaining>0))return 0;
 float seconds=v.remaining/60.f;int n=(int)seconds;if((float)n<seconds)++n;return (float)n;
}
static void* countdown_file(){
 void*f=(B*)fairy_state()+0x3000;
 if(!at<I>(f,0x8c)){
  void*part=(B*)f+0x400;void*sprite=(B*)f+0x440;
  at<I>(part,0)=4;at<float>(part,8)=100;at<I>(part,12)=1;
  at<float>(part,16)=-8;at<float>(part,20)=-24;
  at<I>(part,32)=0xffffffff;at<I>(part,36)=1;
  at<float>(sprite,0)=at<float>(sprite,4)=at<float>(sprite,8)=1;
  at<float>(sprite,16)=at<float>(sprite,20)=at<float>(sprite,24)=1;
  at<float>(sprite,40)=15;at<I>(sprite,44)=1;
  at<void*>(f,0x490)=part;at<void*>(f,0x498)=sprite;
  at<void*>(f,0x58)=(B*)f+0x490;at<void*>(f,0x60)=(B*)f+0x498;
  at<I>(f,0x8c)=at<I>(f,0x90)=1;
 }
 return f;
}
extern "C" void fairy_hud_attach(void*p){
 if(!p||at<I>(p,0x600)!=3)return;
 void*root=at<void*>(p,0x6d8);if(!root)return;
 void*c=at<void*>(root,0x58);for(int n=0;c&&n<32;n++,c=at<void*>(c,0x50))if(at<U>(c,0x98)==(U)&fairy_countdown_value)return;
 void*h=fn<void*(*)(U)>(0x2a17e0)(0xb8);if(!h)return;
 fn<void*(*)(void*,void*,int,void*,void*,void*)>(0x126ae0)(h,countdown_file(),0,(void*)&fairy_countdown_value,p,root);
}
extern "C" void* shield_hud_begin(void*);
extern "C" void shield_hud_end(void*);
extern "C" void fairy_hud_render(void*h,void*renderer,float alpha){
 if(at<U>(h,0x98)==(U)&fairy_countdown_value && !(fairy_countdown_value(at<void*>(h,0xb0),4)>0))return;
 void*prior=shield_hud_begin(h);
 fn<void(*)(void*,void*,float)>(0x12a4e0)(h,renderer,alpha);
 shield_hud_end(prior);
}

// MiniGameExit listens for native ground contact (message 37). Fairy floats
// 10/32 unit above that contact plane. Keep the native button/exit sequence.
struct ExitState {void*pads[64];};
static ExitState& es(){return *(ExitState*)((B*)fairy_state()+0x3800);}
static void forget_exit(void*pad){for(I i=0;i<64;i++)if(es().pads[i]==pad){es().pads[i]=nullptr;release(pad);return;}}
extern "C" void fairy_exit_reset(){for(I i=0;i<64;i++){void*p=es().pads[i];es().pads[i]=nullptr;release(p);}}
extern "C" U fairy_exit_original(void*,int,void*);
extern "C" U fairy_exit_message(void*pad,int msg,void*arg){
 if(msg==4)forget_exit(pad);
 U result=fairy_exit_original(pad,msg,arg);
 if(msg==1 && !(at<I>(pad,0x18)&0x80000001)){
  for(I i=0;i<64;i++)if(es().pads[i]==pad)return result;
  for(I i=0;i<64;i++)if(!es().pads[i]){retain(pad);es().pads[i]=pad;break;}
 }
 return result;
}
extern "C" void fairy_exit_contacts(void*p){
 if(!fairy(p)||!at<void*>(p,0x490))return;
 B mode=at<B>(p,0x1d);if(mode!=0&&mode!=1)return;
 void*gc=(void*)(fairy_base()+0x1d116f0);
 if(at<I>(gc,0)!=0||at<void*>(gc,0x160))return;
 float d=at<float(*)(void*)>(at<void*>(p,0),0x30)(p);if(!(d>0&&d<1000))return;
 Vec foot=world_position(p);foot.y+=.3125f;
 for(I i=0;i<64;i++){
  void*pad=es().pads[i];if(!pad||at<I>(pad,0x18)&0x80000001||at<B>(pad,0x1d9))continue;
  // Respect another player's existing claim to the pad.
  void*owner=at<void*>(pad,0x1e0);if(owner&&owner!=p)continue;
  Vec center=world_position(pad);float x=foot.x-center.x,z=foot.z-center.z,y=foot.y-center.y;
  if(y<-.20f||y>.20f||x*x+z*z>.2025f)continue;
  fairy_exit_original(pad,37,p);
 }
}

// Resolve the character-book description each time it is requested.
extern "C" const char* fairy_description_key(const char* key){
 const char* match="FairyCharacterInfo_14";
 if(!key||!shield_enabled())return key;
 for(I i=0;;i++){if(key[i]!=match[i])return key;if(!match[i])break;}
 return "FairyShieldCharacterInfo_14";
}
struct DescriptionKey{const char*key;I hash;unsigned short length;unsigned short reserved;};
extern "C" const char* fairy_table_original(void*,const DescriptionKey*,int);
extern "C" const char* fairy_table_string(void* table,const DescriptionKey* key,int language){
 if(!key)return fairy_table_original(table,key,language);
 const char* replacement=fairy_description_key(key->key);
 if(replacement==key->key)return fairy_table_original(table,key,language);
 I length=0;while(replacement[length])++length;
 DescriptionKey updated{replacement,fn<I(*)(const char*,int,int)>(0x2dff30)(replacement,length,0),(unsigned short)length,0};
 return fairy_table_original(table,&updated,language);
}

// Native GoBack copies its departing MenuState to +0x588 before the stack pops.
// Keep only its origin metadata; the engine owns and animates the saved page.
struct PageOrigin{void*book;I menu;I parent;};
static PageOrigin* origins(){return (PageOrigin*)((B*)fairy_state()+0x3a00);}
static void remember_page(void*book,I depth){
 if(!book)return;
 PageOrigin* rows=origins();I index=0;for(;index<8;index++)if(rows[index].book==book||!rows[index].book)break;if(index==8)index=0;
 rows[index]={book,0,0};
 if(depth<1||depth>16)return;
 I menu=at<I>(book,0x38+depth*80),parent=at<I>(book,0x38+(depth-1)*80);
 if((menu==115||menu==109)&&parent==133)rows[index]={book,menu,parent};
}
extern "C" void swap_remember_departing(void*book){
 I depth=at<I>(book,0x5d8);remember_page(book,depth);remember_page(at<void*>(book,0x80),depth);
}

struct ShieldUI{void*owner;B texture[32];void*renderer;};
static ShieldUI& shield_ui(){return *(ShieldUI*)((B*)fairy_state()+0x3b00);}
extern "C" const B shield_pixels[];
extern "C" bool shield_health_active(void*p){return p&&active(p)&&alive(p)&&((at<I>(p,0x364)&0x8000)||potion());}
extern "C" void* shield_hud_begin(void*h){
 void*prior=shield_ui().owner;
 shield_ui().owner=at<U>(h,0x98)==fairy_base()+0x1e0b70?at<void*>(h,0xb0):nullptr;
 return prior;
}
extern "C" void shield_hud_end(void*prior){shield_ui().owner=prior;}
extern "C" void shield_set_texture(void*render,void*texture){
 auto&ui=shield_ui();
 if(texture&&at<int>(texture,0)==288&&at<int>(texture,4)==144&&shield_health_active(ui.owner)){
  void*renderer=*(void**)(fairy_base()+0x1dc1e80);
  if(renderer&&ui.renderer!=renderer){
   for(I i=0;i<32;i++)ui.texture[i]=0;
   fn<void*(*)(void*,void*,const B*,int,int,int,int,int,int,const char*)>(0x2e89a0)(renderer,ui.texture,shield_pixels,288,144,1,1,1,1,"Team Swap shield health bar");
   if(at<int>(ui.texture,0)==288&&at<int>(ui.texture,4)==144)ui.renderer=renderer;
  }
  if(renderer&&ui.renderer==renderer)texture=ui.texture;
 }
 fn<void(*)(void*,void*)>(0x2d2af0)(render,texture);
}

extern "C" void portrait_draw_original(void*,void*,float);
extern "C" bool portrait_hidden(void*h){
 return settings_hide_get()&&h&&!at<void*>(h,0x48)&&at<U>(h,0x98)==fairy_base()+0x1e0b70&&at<void*>(h,0xb0);
}
extern "C" void portrait_draw(void*h,void*render,float alpha){
 if(portrait_hidden(h))return;
 portrait_draw_original(h,render,alpha);
}
