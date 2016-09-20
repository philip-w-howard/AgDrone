#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "getimagescmd.h"

#include "log.h"
#include "sendfile.h"

//**********************************************
GetImagesCmd::GetImagesCmd(queue_t *q, int client_socket, int msg_src) :
    CommandProcessor(q, client_socket, msg_src),
    m_sender(m_client_socket)
{
    m_files_found = 0;
    m_files_fetched = 0;
    m_files_sent = 0;
    m_processing = false;

    m_file_queue = queue_create();
    assert(m_file_queue != NULL);
}

//**********************************************
GetImagesCmd::~GetImagesCmd()
{
    m_processing = false;
    gp_camera_exit(m_camera, m_camera_context);
}

//**********************************************
static void *thread_routine(void *param)
{
    ((GetImagesCmd *)param)->SendImagesThread();
    return NULL;
}
//**********************************************
void GetImagesCmd::Start()
{
    m_finished = false;
    m_processing = true;

    WriteLog("Processing GetImagesCmd\n");

    GetFileList();

    m_camera_context = gp_context_new();
    gp_camera_new(&m_camera);

    // When using GP_LOG_DEBUG instead of GP_LOG_ERROR above, the
    // init function seems to traverse the entire filesystem on the camera. 
    // This is partly why it takes so long.
    //printf("Camera init.  Takes about 10 seconds.\n");
    int retval = gp_camera_init(m_camera, m_camera_context);
    WriteLog("  Camera Init: %d\n", retval);

    if (pthread_create(&m_send_thread, NULL, thread_routine, this) != 0)
    {
        m_processing = false;
        m_finished = true;
        return;
    }

    ProcessList();
}
//**********************************************
void GetImagesCmd::Abort()
{
    m_processing = false;
}
//**********************************************
void GetImagesCmd::ProcessMessage(mavlink_message_t *msg, int msg_src)
{
}
//**********************************************
void GetImagesCmd::ProcessList()
{

    while (m_processing && m_fileList.size() > 0)
    {
        element_t file = m_fileList.front();
        m_fileList.pop_front();

        WriteLog("Retrieving %s/%s %d\n",
                file.m_folder, file.m_filename, file.m_size);

        CameraFilePath camera_file_path;

        strcpy(camera_file_path.folder, file.m_folder);
        strcpy(camera_file_path.name, file.m_filename);

        CameraFile *imageFile;

        int retval;
        retval = gp_file_new(&imageFile);
        printf("  gp_file_new: %d\n", retval);
        retval = gp_camera_file_get(m_camera, camera_file_path.folder, 
                camera_file_path.name, GP_FILE_TYPE_NORMAL, 
                imageFile, m_camera_context);
        printf("  gp_cmara_file_get: %d\n", retval);

        const char *filedata;
        unsigned long int filesize;

        retval = gp_file_get_data_and_size(imageFile, &filedata, &filesize);
        printf("  data and size: %d %lu\n", retval, filesize);

        /*
        int fd = open(fn, O_CREAT | O_WRONLY, 0644);
        write(fd, filedata, filesize);
        close(fd);
        */

        gp_file_free(imageFile);
        //write(m_client_socket, "sendingfile\n", strlen("sendingfile\n"));
        //m_sender.Start(file.m_filename, filesize);
        //m_sender.Send(filename);

    }

    queue_mark_closed(m_file_queue);
    while (queue_is_open(m_file_queue))
    {
        sleep(1);
    }

    write(m_client_socket, "imagesdone\n", strlen("imagesdone\n"));
    queue_close(m_file_queue);
    m_finished = true;
}

void GetImagesCmd::GetFileList()
{
    char *file_p;
    char *folder_p;
    char folder[256] = "";
    char *size_p;
    int size;
    char input_line[512];
    char *token;

    FILE *dir_list = popen("gphoto2 --list-files", "r");
    if (dir_list == NULL) 
    {
        WriteLog("Unable to get file list\n");
        m_finished = true;
        return;
    }

    while (fgets(input_line, sizeof(input_line), dir_list) != NULL)
    {
        token = strtok(input_line, " \n");
        if (strcmp(token, "There") == 0 &&
            strcmp(strtok(NULL, " \n"), "are") == 0)
        {
            strtok(NULL, "'");
            folder_p = strtok(NULL, "'");
            if (folder_p != NULL) 
            {
                strcpy(folder, folder_p);
                WriteLog("Looking in folder %s\n", folder);
            }
        }
        else if (token[0] == '#')
        {
            file_p = strtok(NULL, " \n");
            if (file_p != NULL && strlen(file_p) > 0)
            {
                strtok(NULL, " \n");
                size_p = strtok(NULL, " \n");
                if (size_p == NULL || strlen(size_p) == 0) 
                    size = 0;
                else
                    size = atoi(size_p);
                WriteLog("Adding to list: %s\n", file_p);
                m_fileList.push_back(element_t(folder, file_p, size));
            }
        }
    }

    pclose(dir_list);

    std::list<element_t>::iterator it;

    WriteLog("Found %d images\n", m_fileList.size());
}
//*****************************************************
void GetImagesCmd::SendImagesThread()
{
    while (m_processing)
    {
        sleep(1);
    }
}
