#include "./core.h"
#include "dlist/list.h"
#include <stdio.h>
#include <string.h>
#include <zip.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/stat.h>

#define OSSA_CORE_VERSION "0.6-AS(D)"

#ifndef OSSA_CORE_MAXHEADER
    #define OSSA_CORE_MAXHEADER 1024
#endif

/* local */
int (*newMessageHandler)(ossaCID cid, ossaMessage mes);
int pluginChatAddMessage(ossaCID cid, ossaMessage mes){
    listAppend(&((struct ossaChat*)(cid))->messages, &mes, sizeof(ossaMessage));
    if(newMessageHandler != 0x0) newMessageHandler(cid, mes);
    return 0;
}int pluginChatAddUser(ossaCID cid, ossaUser mes){
    listAppend(&((struct ossaChat*)(cid))->userlist, &mes, sizeof(ossaUser));
    return 0;
}
char checkValidPlugin(struct ossaPlugin *plugin){
    return 1;
}
char find_settings(const void *target, void *current){

}
char find_user(const void *target, void *current){

}
void defaultLogFunction(char type, const char *format, ...){
    va_list list;
    va_start(list, format);
    vprintf(format, list);
    va_end(list);
}
int notifyProcess(void *pluginptr, ossaCID cid, ossaMessage message){
    struct ossaPlugin *pl = (struct ossaPlugin *)pluginptr;
    
}
// Local variables
void (*logFunction)(char type, const char *format, ...) = defaultLogFunction;
void (*notifyCallback)(struct ossaChat *where, ossaMessage incoming) = 0x0;
/* export */
int setNewMessageHandler(int (*clientNewMessageHandler)(ossaCID cid, ossaMessage mes)){
    newMessageHandler = clientNewMessageHandler;
}
int setNotifyCallback(void (*callback)(struct ossaChat *where, ossaMessage incoming)){
    notifyCallback = callback;
}
int setLogFunction(void (*ossaLog)(char type, const char *format, ...)){
    logFunction = ossaLog;
    return !(logFunction == 0x0);
}
struct ossaChat makeChat(ossastr title, struct ossaPlugin *plugin){
    //Make chat object
    struct ossaChat chat;
    //Check for valid plugin and liinking to plugin
    if(checkValidPlugin(plugin)){
        chat.plugin = plugin;
    }else{
        return (struct ossaChat){0x0, lnothing, lnothing, lnothing, 0x0};
    }
    //Copy title
    chat.title = (malloc(strlen(title)));
    strcpy(chat.title, title);
    //Making empty lists
    chat.messages = makeEmptyList();
    chat.userlist = makeEmptyList();
    ossaUser zero = {"me", "{\"metadata\":{\"visual\":{\"pictype\":\"none\",\"picture\":\"none\"},\"text\":{\"name\":\"Default OSSA user (me)\",\"bio\":\"\"}},\"chat\":{\"name\":\"me\",\"ossauid\":0,\"roles\":[]}\"GID\":\"OUKVp0.4-C\"}"};
    listAppend(&chat.userlist, &zero, sizeof(ossaUser));
    chat.settings = makeEmptyList();
    chat.cid = plugin->pcall.makeChat(title);

    if(chat.cid < 0){
        fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to make new chat\n");
    }

    return chat;
}

int setChatSettings(struct ossaChat* _this, ossastr field, ossastr data){
    // Not found settings field
    listRemove(&_this->settings, listFind(&_this->settings, field, find_settings));
    unsigned int size = strlen(field)+strlen(data);
    char *buffer = malloc(size);
    sprintf(buffer, "%s\r%s", field, data);
    listAppendLink(&_this->settings, buffer);
    return OSSA_OK;
}

ossalist(ossastr) getChatSettings(struct ossaChat* _this){
    return _this->settings;
}

int inviteToChat(struct ossaChat* _this, ossastr globalUID){
    ossaUser newUser = _this->plugin->pcall.globalUIDInfo(globalUID);
    if(newUser.nickname == 0x0){
        //No user
        return OSSA_NOUSER;
    }
    if(listFind(&_this->userlist, &newUser, find_user)){
        return OSSA_ALREADY;
    }
    int code = chatAction(_this, "invite", (ossalist(ossastr)){globalUID, 0x0});
    if(code == OSSA_OK || code == OSSA_ACCPEPT){
        listAppend(&_this->userlist, &newUser, sizeof(ossaUser));
    }
    return code;
}

int deleteUser(struct ossaChat* _this, ossaUID uid, ossastr additional){
    ossalist(ossastr) argv = makeEmptyList();
    int charscount = 1024; // ((ceil(log10(uid))+1)*sizeof(char)); /* ибо пошли нахуй все эти "умные" формулы, сюка" */
    char *j = malloc(charscount+1);
    sprintf(j, "%lu", uid);
    listAppend(&argv, j, charscount+1);
    listAppend(&argv, additional, strlen(additional));
    int code = chatAction(_this, "userdel", argv);
    if(code == OSSA_OK){
        listRemove(&_this->userlist, uid);
    }
    earaseList(&argv);
    return code;
}

int ossSetPluginRoutineDirect(void *pluginptr,void*(*routine)(void *args), void* args){
    pthread_t pid;
    pthread_create(&pid, 0x0, routine, args);
    pthread_detach(pid);
    listAppend(&((struct ossaPlugin*)(pluginptr))->threads, &pid, sizeof(pthread_t));
}

int sendMessage(struct ossaChat *_this, ossaMessage message){
    listAppend(&_this->messages, &message, sizeof(ossaMessage));
    if(!(_this->plugin->pcall.state() & (OSSA_STATE_AUTHED|OSSA_STATE_ENABLE)))
        return OSSA_BAD_LOGIN;
    _this->plugin->pcall.sendMes(_this->cid, message);
    return updateChat(_this);
}

ossaMessage makeMessage(struct ossaChat *_this, ossastr body, ossalist(ossastr) attachments){
    ossaMessage me;
    me.uid = astype(ossaUID) listGet(&_this->userlist, 0); //0th user is always 'me'
    me.body = body;
    me.attach = attachments;
    return me;
}

int editMessage(struct ossaChat *_this, ossaMID mid, ossaMessage edited){
    astype(ossaMessage) listGet(&_this->userlist, mid) = edited;
    return updateChat(_this);
}

int chatAction(struct ossaChat *_this, ossastr action_name, ossalist(ossastr) args){
    char *argv = malloc(5120);
    memset(argv, 0, 5120);
    strcpy(argv, action_name);
    for(int i = 0; i < listLen(&args); i++){
        sprintf(argv, "%s %s", argv, (char*)listGet(&args, i));
        // strcat(argv, );
    }
    int ret = _this->plugin->pcall.chatAction(_this->cid, argv);
    free(argv);
    return ret;
}

int updateChat(struct ossaChat *_this){
    if( _this->plugin->pcall.updateChat(_this->cid) > 0){
        
    }
    return _this->plugin->pcall.updateChat(_this->cid);
}

int exportChat(struct ossaChat *_this, ossastr location){
    struct stat stats;
    ossastr final = location;

    if(stat(location, &stats) == 0){
        if(S_ISDIR(stats.st_mode)){
            //if it directory, we should place
            final = malloc(strlen(location)+strlen("/drop.ossadrop"));
            sprintf(final, "%s/drop.ossadrop", location);
        }
    }else{
        // fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to stat \'%s\'!\n");
    }
    int err = 0;
    zip_t *drop = zip_open(location, ZIP_CREATE, &err);
    if(err != 0){
        fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to open \'%s\' for writing, %s", location, zip_strerror(drop));
        return err;
    }
    { //Writing header
        char headerdata[OSSA_CORE_MAXHEADER];
        sprintf(headerdata, "{\"desc\":\"OSSA Chat Archive Header File\",");
        sprintf(headerdata, "\"OSSA Core version\":\"%s\",", OSSA_CORE_VERSION);
        sprintf(headerdata, "\"messages_count\":%i,", listLen(&_this->messages));
        sprintf(headerdata, "\"users_count\":%i,", listLen(&_this->userlist));
        sprintf(headerdata, "\"plugin\":{");
        sprintf(headerdata, "\"name\":\"%s\",", _this->plugin->name);
        sprintf(headerdata, "\"location\":\"%s\",", _this->plugin->loaction);
        sprintf(headerdata, "},");
        sprintf(headerdata, "\"settings\":[");
        for(int i = 0; i < listLen(&_this->settings); i++){
            sprintf(headerdata, "\"%s\",", (char*)listGet(&_this->settings, i));
        }
        sprintf(headerdata, "]}");
        zip_source_t *s;
        if((s = zip_source_buffer(drop, headerdata, sizeof(headerdata), 0)) == 0x0||
        zip_file_add(drop, "header.json", s, ZIP_FL_ENC_UTF_8) < 0){
            zip_source_free(s);
            fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to write header: %s",\
                zip_strerror(drop));
        }
    }
    zip_close(drop);
    return 0;
}

int loadChatPlugin(struct ossaPlugin *_this, ossastr path){
    if(path == 0x0){
        return -1;
    }
    void *entity = _this->libEntity = dlopen(path, RTLD_LAZY);
    if(_this->libEntity == 0x0){
        fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to open \'%s\' plugin: \n\t%s\n", path, dlerror());
        return -1;
    }
    unsigned int nullCounter = 0;
    _this->loaction = malloc(strlen(path));
    strcpy(_this->loaction, path);
    _this->name = *((char**)(dlsym(entity, "plugin_name")));
    if(_this->name == 0x0){
        return -2;
    }
    for(int i = 0; _this->name[i] != 0; i++){
        if(((_this->name[i] > 'z' || _this->name[i] < 'a')&&
            (_this->name[i] > 'Z' || _this->name[i] < 'A')&&
            (_this->name[i] > '9' || _this->name[i] < '0'))&&
            (_this->name[i] != '_' && _this->name[i] != '-')){
                fprintf(stderr, "Unregular name: \'%s\', \'%c\'\n", _this->name, _this->name[i]);
                return -2;
            }
    }
    if(dlsym(entity, "self") == 0x0){
        return -3;
    }
    *((void**)(dlsym(entity, "self"))) = (void*)_this;
    if(_this->name == 0x0) {
        fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to load \'%s\' plugin: unnamed plugin.\n", path);
    }
    if(dlsym(entity, "ossaLog") != 0x0){
        *((void(**)(char, const char*, ...))dlsym(entity, "ossaLog")) = logFunction;
    }
    _this->init = dlsym(entity, "plugin_init");
    _this->pcall.connect = (int(*)())dlsym(entity, "plugin_connect");
    _this->pcall.disconnect = (int(*)())dlsym(entity, "plugin_disconnect");
    _this->pcall.state = (int(*)())dlsym(entity, "plugin_state");

    _this->pcall.auth = (int(*)(ossastr, ossastr))dlsym(entity, "plugin_user_auth");
    _this->pcall.oauth = (int(*)(ossastr))dlsym(entity, "plugin_user_oauth");
    _this->pcall.exit = (int(*)())dlsym(entity, "plugin_user_exit");
    _this->pcall.renameMe = (int(*)(ossastr))dlsym(entity, "plugin_user_rename");
    _this->pcall.myInfo = (ossaUser(*)())dlsym(entity, "plugin_user_info");
    _this->pcall.globalUIDInfo = (ossaUser(*)(ossastr))dlsym(entity, "plugin_user_ginfo");

    _this->pcall.sendMes = (int(*)(ossaCID, ossaMessage))dlsym(entity, "plugin_message_send");
    _this->pcall.editMes = (int(*)(ossaCID, ossaMessage, ossaMID))dlsym(entity, "plugin_message_edit");

    _this->pcall.makeChat = (ossaCID(*)(ossastr))dlsym(entity, "plugin_chat_makeChat");
    _this->pcall.getChatSettings = (ossastr(*)(ossaCID))dlsym(entity, "plugin_chat_getprefs");
    _this->pcall.setChatSettings = (int(*)(ossaCID, ossastr, ossastr))dlsym(entity, "plugin_chat_setpref");
    _this->pcall.updateChat = (int(*)(ossaCID))dlsym(entity,"plugin_chat_update");
    _this->pcall.loadChat = (int(*)(ossaCID, ossastr))dlsym(entity,"plugin_chat_load");
    _this->pcall.getChatList = (ossastr(*)())dlsym(entity,"plugin_chat_list");
    _this->pcall.getChatGUIDs = (ossastr(*)(ossaCID))dlsym(entity, "plugin_chat_getGUIDs");
    _this->pcall.chatAction = (int(*)(ossaCID, ossastr))dlsym(entity, "plugin_chat_action");

    { /* check for NULL functions */
        if(_this->init == 0x0) nullCounter++;

        if(_this->pcall.connect == 0x0) nullCounter++;
        if(_this->pcall.disconnect == 0x0) nullCounter++;
        if(_this->pcall.state == 0x0) nullCounter++;
        
        if(_this->pcall.auth == 0x0) nullCounter++;
        if(_this->pcall.oauth == 0x0) nullCounter++;
        if(_this->pcall.exit == 0x0) nullCounter++;
        if(_this->pcall.renameMe == 0x0) nullCounter++;
        if(_this->pcall.myInfo == 0x0) nullCounter++;
        if(_this->pcall.globalUIDInfo == 0x0) nullCounter++;

        if(_this->pcall.sendMes == 0x0) nullCounter++;
        if(_this->pcall.editMes == 0x0) nullCounter++;

        if(_this->pcall.makeChat == 0x0) nullCounter++;
        if(_this->pcall.getChatSettings == 0x0) nullCounter++;
        if(_this->pcall.setChatSettings == 0x0) nullCounter++;
        if(_this->pcall.updateChat == 0x0) nullCounter++;
        if(_this->pcall.loadChat == 0x0) nullCounter++;
        if(_this->pcall.getChatList == 0x0) nullCounter++;
        if(_this->pcall.getChatGUIDs == 0x0) nullCounter++;
    }
    { //exporting
        void *ptr = 0x0;
        //ossaClientSetRoutineDirect
        ptr = dlsym(entity, "ossaClientSetRoutineDirect");
        if(ptr != 0x0){
            *((int(**)(void*,void*(*)(void*), void*))(ptr)) = ossSetPluginRoutineDirect;
        }
        ptr = 0x0;
        ptr = dlsym(entity, "ossaChatAddMessage");
        if(ptr != 0x0){
            *((int(**)(ossaCID cid, ossaMessage))(ptr)) = pluginChatAddMessage;
        }
        ptr = dlsym(entity, "ossaChatAddUser");
        if(ptr != 0x0){
            *((int(**)(ossaCID cid, ossaUser))(ptr)) = pluginChatAddUser;
        }
    }

    if(_this->init == 0x0){
        fprintf(stderr, "[!!] OSSA Core: Fatal error: failed to load init\n");
        return -2;
    }else{
        int code = _this->init();
        if(code != OSSA_OK){
            fprintf(stderr, "[!!] OSSA Core: Fatal error: init failed with code %i\n", code);
            return -3;
        }
    }

    return nullCounter;
}

#ifndef COMPILE_STATIC
    // #warning "USING DYNAMIC COMPILATION"
    int main(){

    }
#else
    // #warning "USING STATIC COMPILATION"
#endif

ossastr getUsernameFromUser(ossaUser user){
    return user.nickname;
}