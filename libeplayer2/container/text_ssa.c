#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>

static const char FILENAME[] = "text_ssa.c";
#define TEXTSSAOFFSET 200

#include "common.h"
static pthread_t thread_sub = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    char * File;
    int Id;
} SsaTrack_t;

#define TRACKWRAP 20
static SsaTrack_t * Tracks;
static int TrackCount = 0;
static int CurrentTrack = -1; //no as default.

static void SsaManagerAdd(Context_t  *context, SsaTrack_t track) {
	printf("%s::%s %s %d\n", FILENAME, __FUNCTION__, track.File, track.Id);

	if (Tracks == NULL) {
		Tracks = malloc(sizeof(SsaTrack_t) * TRACKWRAP);
	}
	
	if (TrackCount < TRACKWRAP) {
		printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
		Tracks[TrackCount].File       = strdup(track.File);
        Tracks[TrackCount].Id         = track.Id;
        TrackCount++;
	} else {
		//Track_t * tmp_Tracks = realoc(sizeof(Tracks));
	}
}

static char ** SsaManagerList(Context_t  *context) {
	char ** tracklist = NULL;
	printf("%s::%s\n", FILENAME, __FUNCTION__);
	if (Tracks != NULL) {
		int i = 0, j = 0;
		tracklist = malloc(sizeof(char *) * ((TrackCount*2) + 1));
		for (i = 0, j = 0; i < TrackCount; i++, j+=2) {
		printf("2strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
            tracklist[j]    = strdup(Tracks[i].Id);
			tracklist[j+1]    = strdup(Tracks[i].File);
		}
        tracklist[j] = NULL;
	}

	return tracklist;
}

static void SsaManagerDel(Context_t * context) {

    int i = 0;
    if(Tracks != NULL) {
        for (i = 0; i < TrackCount; i++) {
		    free(Tracks[i].File);
            Tracks[i].File = NULL;
	    }
	    free(Tracks);
        Tracks = NULL;
    }

    TrackCount = 0;
    CurrentTrack = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static void getExtension(char * FILENAMEname, char ** extension) {

	int i = 0;
	int stringlength = (int) strlen(FILENAMEname);
	printf("%s::%s\n", FILENAME, __FUNCTION__);
	for (i = 0; stringlength - i > 0; i++) {
		if (FILENAMEname[stringlength - i - 1] == '.') {
			printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
			*extension = strdup(FILENAMEname+(stringlength - i));
			break;
		}
	}
}

static void getParentFolder(char * Filename, char ** folder) {
	printf("%s::%s\n", FILENAME, __FUNCTION__);

	int i = 0;
	int stringlength = strlen(Filename);

	for (i = 0; stringlength - i > 0; i++) {
		if (Filename[stringlength - i - 1] == '/') {
            char* sTmp = (char *)malloc(stringlength - i);
			strncpy(sTmp, Filename, stringlength - i - 1);
            sTmp[stringlength - i -1] = '\0';
		printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
            *folder = strdup(sTmp);
            free(sTmp);
			break;
		}
	}
}

static void SsaGetSubtitle(Context_t  *context, char * Filename) {
	printf("%s::%s\n", FILENAME, __FUNCTION__);

    struct dirent *dirzeiger;
    DIR  *  dir;
    int     i                    = TEXTSSAOFFSET;
    char *  FilenameExtension    = NULL;
    char *  FilenameFolder       = NULL;
    char *  FilenameShort        = NULL;
    int     FilenameLength          = 0;
    int     FilenameShortLength     = 0;
    int     FilenameFolderLength    = 0;
    int     FilenameExtensionLength = 0;

    FilenameLength = strlen(Filename);

    //printf("%s::%s %s %d\n", FILENAME, __FUNCTION__, Filename, FilenameLength);

    getParentFolder(Filename, &FilenameFolder);
    FilenameFolderLength = strlen(FilenameFolder);

    //printf("%s::%s %s %d\n", FILENAME, __FUNCTION__, FilenameFolder, FilenameFolderLength);

    getExtension(Filename, &FilenameExtension);
    FilenameExtensionLength = strlen(FilenameExtension);

    //printf("%s::%s %s %d\n", FILENAME, __FUNCTION__, FilenameExtension, FilenameExtensionLength);

    FilenameShortLength = FilenameLength - FilenameFolderLength - 1 /* / */ - FilenameExtensionLength - 1 /* . */;
    FilenameShort = (char*) malloc(FilenameShortLength + 1 /* \0 */);
    strncpy(FilenameShort, Filename + (strlen(FilenameFolder) + 1 /* / */), FilenameShortLength);
    FilenameShort[FilenameShortLength] = '\0';

    //printf("%s::%s %s %d\n", FILENAME, __FUNCTION__, FilenameShort, FilenameShortLength);

    //printf("%s\n%s | %s | %s\n", Filename, FilenameFolder, FilenameShort, FilenameExtension);

    if((dir=opendir(FilenameFolder)) != NULL) {
        while((dirzeiger=readdir(dir)) != NULL) {
            char * extension = NULL;

            //printf("%s\n",(*dirzeiger).d_name);
            getExtension((*dirzeiger).d_name, &extension);

            if(extension != NULL) {
                if(!strncmp(extension, "ssa", 3)) {
                    //often the used language is added before .ssa so check
                    char * name = NULL;
                    char * subExtension = NULL;
                    char * language = NULL;
                    int nameLength = strlen((*dirzeiger).d_name) - 4;

                    name = (char*) malloc(nameLength + 1);
                    strncpy(name, (*dirzeiger).d_name, nameLength);
                    name[nameLength] = '\0';
                    
                    getExtension(name, &subExtension);
                    printf("%s %s\n",name, subExtension);

                    if(subExtension == NULL) {
			printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
                        language = strdup("und");

                    } else {
                        int tmpLength = strlen(name) /*- strlen(subExtension) - 1*/;
                        char * tmpName = (char*) malloc(tmpLength + 1);
                        strncpy(tmpName, name, tmpLength);
                        tmpName[tmpLength] = '\0';

                        free(name);
			printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
                        name = strdup(tmpName);
                        free(tmpName);
			printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
                        language = strdup(subExtension);
                        free(subExtension);
                    }

                    printf("%s %s %d\n",name, FilenameShort, FilenameShortLength);

                    if(!strncmp(name, FilenameShort, FilenameShortLength)) {
                        char * absSubtitleFilenam = (char*) malloc(strlen(FilenameFolder) + 1 + strlen((*dirzeiger).d_name));
                        strcpy(absSubtitleFilenam, FilenameFolder);
                        strcat(absSubtitleFilenam, "/");
                        strcat(absSubtitleFilenam, (*dirzeiger).d_name);

                        SsaTrack_t SsaSubtitle = {
				            absSubtitleFilenam,
				            i,
			            };
                        SsaManagerAdd(context, SsaSubtitle);

                        Track_t Subtitle = {
				            language,
				            "S_TEXT/SSA",
				            i++,
			            };
                        context->manager->subtitle->Command(context, MANAGER_ADD, &Subtitle);
                    }

                }

                free(extension);
            }
        }
    }

    free(FilenameExtension);
    free(FilenameFolder);
    free(FilenameShort);

}

FILE * fssa = NULL;
#define MAXLINELENGTH 1000

static int SsaOpenSubtitle(Context_t *context, int trackid) {

    if(trackid < TEXTSSAOFFSET) {
        return -1;
    }

    trackid %= TEXTSSAOFFSET;

    printf("%s::%s %s\n", FILENAME, __FUNCTION__, Tracks[trackid].File);

    fssa = fopen(Tracks[trackid].File, "rb");
    printf("%s::%s %s\n", FILENAME, __FUNCTION__, fssa?"fssa!=NULL":"fssa==NULL");
    if(!fssa)
        return -1;

    return 0;
}

static int SsaCloseSubtitle(Context_t *context) {

    if(fssa)
        fclose(fssa);

    fssa = NULL;

    return 0;
}
static long int numScene = 0;

//Buffer size used in getLine function. Do not set to value less than 1 !!!
#define BUFFER_SIZE 14

char *SSAgetLine(FILE **fssa) 
{
   char *strAux = NULL, *strInput;
   char c[BUFFER_SIZE], ch;
   int k, tam, tamAux;   

   k = tamAux = 0;

   if(BUFFER_SIZE>0)
   { 

     strInput = (char*)malloc(1*sizeof(char));
     strInput[0]='\0';

      while(tamAux!=1)
      {

	 if((ch = fgetc(*fssa))!=EOF)
         {
           ungetc(ch , *fssa);
           fgets(c, BUFFER_SIZE, *fssa);
           strAux = (char*)strchr(c,'\n');
           tam = strlen(c);
           if(strAux != NULL)
           {
               tamAux = strlen(strAux);
	       tam--;
           }
	   
           k = k + tam;
           strInput = (char*)realloc(strInput, (k+1)*sizeof(char));

           if(k!=tam)
               strncat(strInput, c, tam);
           else
               strncpy(strInput, c, tam);

           strInput[k] = '\0';      

	 }
	 else  tamAux = 1;
      }
               
  }

  return strInput;   

}

static void SsaSubtitleThread(Context_t *context) {
	printf("%s::%s\n", FILENAME, __FUNCTION__);
    int pos = 0;
    char  Data[MAXLINELENGTH];
    unsigned long long int Pts = 0;
    float Duration = 0;
    char * Text = NULL;

    while(context->playback->isPlaying && fssa /*&& fgets(Data, MAXLINELENGTH, fssa)*/) {
	//printf("%s::%s \n", FILENAME, __FUNCTION__);
		int comma, lines;
		static int max_comma = 32; /* let's use 32 for the case that the */
                    			   /*  amount of commas increase with newer SSA versions */

		int hour1, min1, sec1, hunsec1,
		hour2, min2, sec2, hunsec2, nothing;
		int num;
		char *line = NULL;
		char line3[MAXLINELENGTH+1], *line2;
		char *tmp;

		do {
			line = SSAgetLine(&fssa);
		} while (sscanf (line, "Dialogue: Marked=%d,%d:%d:%d.%d,%d:%d:%d.%d,"
			"%[^\n\r]", &nothing,
			&hour1, &min1, &sec1, &hunsec1, 
			&hour2, &min2, &sec2, &hunsec2,
			line3) < 9
		 &&
		 sscanf (line, "Dialogue: %d,%d:%d:%d.%d,%d:%d:%d.%d,"
			 "%[^\n\r]", &nothing,
			 &hour1, &min1, &sec1, &hunsec1, 
			 &hour2, &min2, &sec2, &hunsec2,
			 line3) < 9	    );

		line2=strchr(line3, ',');
		for (comma = 4; comma < max_comma; comma ++)
		{
			tmp = line2;
			if(!(tmp=strchr(++tmp, ','))) break;
			if(*(++tmp) == ' ') break; 
			/* a space after a comma means we're already in a sentence */
			line2 = tmp;
		}

		if(comma < max_comma)max_comma = comma;
		/* eliminate the trailing comma */
		if(*line2 == ',') line2++;

		Pts = (hour1*3600 + min1*60 + sec1)*1000 + hunsec1;
		Duration = (hour2*3600 + min2*60 + sec2) * 1000  + hunsec2 - Pts;
		Pts = Pts * 90;
		printf("strdup in %s::%s:%d\n", FILENAME, __FUNCTION__,__LINE__);
		Text=strdup(line2);
                context->output->subtitle->Write(context, Text, strlen(Text), Pts, NULL, 0, Duration, "subtitle");

                free(Text);
                Text = NULL;
                continue;
    }
    if(Text) {
        free(Text);
        Text = NULL;
    }

}

static int SsaPlay(Context_t *context) {
    printf("%s::%s\n",  FILENAME, __FUNCTION__);

    int curtrackid = -1;
    context->manager->subtitle->Command(context, MANAGER_GET, &curtrackid);

    if(SsaOpenSubtitle(context, curtrackid) == 0){
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create (&thread_sub, &attr, &SsaSubtitleThread, context);   
    }

    return 0;
}

static int SsaSwitchSubtitle(Context_t *context, int* arg) {

    SsaCloseSubtitle(context);
    if(SsaOpenSubtitle(context, *arg)==0){
	 pthread_attr_t attr;
	 pthread_attr_init(&attr);
	 pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
         pthread_create (&thread_sub, &attr, &SsaSubtitleThread, context);   
    }

    return 0;
}

static int SsaDel(Context_t *context) {

    SsaCloseSubtitle(context);
    SsaManagerDel(context);
    return 0;
}

static int Command(Context_t  *context, ContainerCmd_t command, void * argument) {
	printf("%s::%s\n", FILENAME, __FUNCTION__);

	switch(command) {
		case CONTAINER_INIT: {
			char * filename = (char *)argument;
			SsaGetSubtitle(context, filename);
			break;
		}
		case CONTAINER_PLAY: {
			SsaPlay(context);
			break;
		}
		case CONTAINER_DEL: {
			SsaDel(context);
			break;
		}
		case CONTAINER_SWITCH_SUBTITLE: {
            printf("%s::%s CONTAINER_SWITCH_SUBTITLE id=%d\n", FILENAME, __FUNCTION__, *((int*) argument));

		    SsaSwitchSubtitle(context, (int*) argument);
			break;
		}
		default:
			printf("%s::%s ConatinerCmd not supported! %d\n", FILENAME, __FUNCTION__, command);
			break;
	}

	return 0;
}

static char *SsaCapabilities[] = { "ssa", NULL };

Container_t SsaContainer = {
	"SSA",
	&Command,
	SsaCapabilities,
};