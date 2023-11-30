#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#ifdef LINUX

#endif

#ifdef MACOS
#define BACKSPACE 127
#endif

#ifdef WINDOWS
#define BACKSPACE 8
#endif

#define BACKSPACE 127
#define ENTER 10
#ifndef CTRL
#define CTRL(c) ((c) & 037)
#endif

typedef struct Node
{
    struct Node *prev;
    struct Node *next;
    int data;
} Node;

typedef struct Position
{
    int x;
    int y;
    Node *current;
} Position;

typedef struct WindowSize
{
    int x;
    int y;
} WindowSize;

typedef struct DocumentInfo
{
    int lineCount;

    Node *frameFirstNode;
    Node *frameLastNode;
    int frameY;
} DocumentInfo;

typedef struct fileInfo
{
    char *filename;
    char *filetype;
    bool isFileReading;
    bool isUpdated;
    bool isNewFile;
} FileInfo;

Node *head;
Node *tail;
Position *position;
WindowSize *windowSize;
DocumentInfo *documentInfo;
FileInfo *fileInfo;

// related to lines
void print(void);

// initalize
void initLinkedList(void);
void initCurses(void);
void initWindowSize(void);
void initDocumentInfo(void);

// linked list
void insert(int data);
void delete(void);

// row length
int current_row_length(void);
int prev_row_length(void);
int next_row_length(void);

// key
void backspace(void);
void enter(void);
void arrow(void);
void save(void);
void commonKey(int key);

void moveFirstFrameRight(void);
void moveLastFrameLeft(void);
void moveFirstFrameLeft(void);
void moveLastFrameRight(void);

void initLinkedList(void)
{
    head = (Node *)malloc(sizeof(Node));
    tail = (Node *)malloc(sizeof(Node));
    position = (Position *)malloc(sizeof(Position));
    position->current = head;
    position->x = 0;
    position->y = 0;

    head->data = 0;
    tail->data = 0;

    head->next = tail;
    head->prev = head;
    tail->next = tail;
    tail->prev = head;
}

void initCurses(void)
{
    initscr();
    start_color();
    init_color(1, 0, 0, 0);
    init_color(2, 1000, 1000, 1000);
    init_pair(1, 1, 2);
    noecho();
    move(0, 0);
}

void initWindowSize(void)
{
    windowSize = (WindowSize *)malloc(sizeof(WindowSize));
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    windowSize->x = (int)w.ws_col;
    windowSize->y = (int)w.ws_row;
}

void initDocumentInfo(void)
{
    documentInfo = (DocumentInfo *)malloc(sizeof(DocumentInfo));
    documentInfo->lineCount = 1;

    documentInfo->frameFirstNode = (Node *)malloc(sizeof(Node));
    documentInfo->frameLastNode = (Node *)malloc(sizeof(Node));

    documentInfo->frameFirstNode = head;
    documentInfo->frameLastNode = tail;
    documentInfo->frameY = 0;
}

void initFileInfo(void)
{
    fileInfo = (FileInfo *)malloc(sizeof(FileInfo));
    fileInfo->filename = "No name";
    fileInfo->filetype = "no ft";
    fileInfo->isUpdated = false;
    fileInfo->isNewFile = true;
}
void insert(int data)
{
    Node *new_node = (Node *)malloc(sizeof(Node));
    Node *p = position->current;

    new_node->data = data;
    new_node->next = p->next;
    new_node->prev = p;

    p->next->prev = new_node;
    p->next = new_node;

    position->current = new_node;
}

void delete()
{
    Node *prev = position->current->prev;    
    position->current->prev->next = position->current->next;
    position->current->next->prev = prev;

    free(position->current);
    position->current = prev;
}

int current_row_length(void)
{
    Node *p1 = position->current;
    Node *p2 = position->current->next;
    // if (p1->data == ENTER && p1->next->data == ENTER)
    //     return 1;
    int count = 0;
    while ((p1->data != ENTER) && (p1 != head))
    {
        p1 = p1->prev;
        count++;
    }
    while ((p2->data != ENTER) && (p2 != tail))
    {
        p2 = p2->next;
        count++;
    }
    return count;
}

int prev_row_length(void)
{
    Node *p = position->current;
    int count = 0;
    while ((p->data != ENTER) && (p != head))
        p = p->prev;

    if (p == head)
        return -1;

    p = p->prev;
    while ((p->data != ENTER) && (p != head))
    {
        p = p->prev;
        count++;
    }
    return count;
}

int next_row_length(void)
{
    Node *p = position->current;

    int count = 0;
    if (p->data == ENTER)
        p = p->next;
    while ((p->data != ENTER) && (p != tail))
        p = p->next;

    if (p == tail)
        return -1;

    p = p->next;
    while ((p->data != ENTER) && (p != tail))
    {
        p = p->next;
        count++;
    }
    return count;
}


void print()
{
    if (fileInfo->isFileReading)
        return;
    clear();
    Node *p = documentInfo->frameFirstNode;
    while (p->next != documentInfo->frameLastNode && p->next != tail)
    {
        addch(p->next->data);
        p = p->next;
    }

    attron(COLOR_PAIR(1));

    char leftMessage[100];
    char rightMessage[100];

    sprintf(leftMessage, "[%s] - %d lines ffn=%c fln=%c prl = %d crl = %d nrl = %d",
            fileInfo->filename, documentInfo->lineCount,
            documentInfo->frameFirstNode->next->data,
            documentInfo->frameLastNode->prev->data,
            prev_row_length(),
            current_row_length(),
            next_row_length());
    sprintf(rightMessage, "%s | %d/%d",
            fileInfo->filetype,
            position->y + 1, position->x);

    mvprintw(windowSize->y - 2, 0, "%s", leftMessage);
    mvprintw(windowSize->y - 2, windowSize->x - strlen(rightMessage), "%s", rightMessage);
    for (int i = strlen(leftMessage); i < windowSize->x - strlen(rightMessage); ++i)
    {
        mvaddch(windowSize->y - 2, i, ' ');
    }
    attroff(COLOR_PAIR(1));

    mvprintw(windowSize->y - 1, 0, "HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    // 글이 없는 경우에는 ~표시를 하기
    for(int i=0;i<windowSize->y-documentInfo->lineCount-2;i++) {
        mvprintw(windowSize->y-3-i, 0, "~");
    }
    move(position->y, position->x);
}

void moveFirstFrameRight(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameFirstNode = documentInfo->frameFirstNode->next;
    while (documentInfo->frameFirstNode->data != ENTER )
    {
        documentInfo->frameFirstNode = documentInfo->frameFirstNode->next;
    }
}

void moveFirstFrameLeft(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameFirstNode = documentInfo->frameFirstNode->prev;
    while (documentInfo->frameFirstNode->data != ENTER && documentInfo->frameFirstNode != head)
    {
        documentInfo->frameFirstNode = documentInfo->frameFirstNode->prev;
    }
}

void moveLastFrameRight(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameLastNode = documentInfo->frameLastNode->next;
    while (documentInfo->frameLastNode->data != ENTER && documentInfo->frameLastNode != tail)
    {
        documentInfo->frameLastNode = documentInfo->frameLastNode->next;
    }
}

void moveLastFrameLeft(void)
{
    if (fileInfo->isFileReading)
        return;
    documentInfo->frameLastNode = documentInfo->frameLastNode->prev;
    while (documentInfo->frameLastNode->data != ENTER)
    {
        documentInfo->frameLastNode = documentInfo->frameLastNode->prev;
    }
}

void backspace(void)
{
    if (position->current == head)
        return;

    if (position->x == 0 && position->y == 0)
    { // 페이지의 제일 처음인 경우 (문서 처음 x)
        int prl = prev_row_length();
        move(0, prl);
        position->x = prl;
        moveFirstFrameLeft();
        moveLastFrameLeft();
        documentInfo->frameY--;
        documentInfo->lineCount--;
    }
    else if (position->x == 0)
    { // 위 경우를 제외하고 커서가 라인의 제일 처음인 경우
        int prl = prev_row_length();
        
        if(documentInfo->frameLastNode == tail && documentInfo->frameY != 0) {
            moveFirstFrameLeft();
            documentInfo->frameY --;
            
            move(position->y, prl);
            position->x = prl;    
            
        } else {
            move(--position->y, prl);
            position->x = prl;
            moveLastFrameRight();
        }
        
        documentInfo->lineCount--;


    }
    else
    {
        move(position->y, --position->x);
    }
    delete ();
    print();
}

void enter(void)
{
    insert(ENTER);
    documentInfo->lineCount++;
    position->x = 0;
    if (documentInfo->lineCount >= windowSize->y - 1) // 문서의 라인 수가 화면 크기보다 클 경우
    {
        if (position->y == windowSize->y - 3)
        { // 커서가 화면의 제일 밑인 경우
            move(position->y, 0);
            moveFirstFrameRight();
            moveLastFrameRight();
            documentInfo->frameY++;
        }
        else
        {
            move(++position->y, 0);
            moveLastFrameLeft();
        }
    }
    else
    { // 이외의 경우
        move(++position->y, 0);
    }
    print();
}

void arrow(void)
{
    getch();
    switch (getch())
    {
    case 'A': // arrow up
        if (position->y == 0 && documentInfo->frameY == 0)
            break;
        int prl = prev_row_length();
        if (prl > position->x)
        { // 윗줄이 커서의 x값보다 긴 경우
            for (int i = 0; i < prl + 1; i++)
            {
                position->current = position->current->prev;
            }
            move(--position->y, position->x);
        }
        else
        { // 윗줄이 커서의 x값보다 짧은 경우
            for (int i = 0; i < position->x + 1; i++)
            {
                position->current = position->current->prev;
            }
            move(--position->y, prl);
            position->x = prl;
        }
        if (position->y < 0)
        {
            position->y = 0;
            moveFirstFrameLeft();
            moveLastFrameLeft();
            documentInfo->frameY--;
        }
        break;
    case 'B': // arrow down
        if (position->y + documentInfo->frameY == documentInfo->lineCount)
            break;
        int crl = current_row_length();
        int nrl = next_row_length();
        if (nrl > position->x)
        {
            for (int i = 0; i < crl + 1; i++)
            { // 아래 줄이 커서의 x값보다 긴 경우
                position->current = position->current->next;
            }
            move(++position->y, position->x);
        }
        else
        { // 아래 줄이 커서의 x값보다 짧은 경우
            for (int i = 0; i < crl - position->x + nrl + 1; i++)
            {
                position->current = position->current->next;
            }
            move(++position->y, nrl);
            position->x = nrl;
        }
        if (position->y > windowSize->y - 3)
        {
            position->y--;
            moveFirstFrameRight();
            moveLastFrameRight();
            documentInfo->frameY++;
        }
        break;
    case 'C': // arrow right
        if (position->current->next == tail)
            break;
        position->current = position->current->next;
        if (position->current->data == ENTER)
        {
            move(++position->y, 0);
            position->x = 0;
        }
        else
        {
            move(position->y, ++position->x);
        }
        if (position->y == windowSize->y - 2)
        {
            position->y--;
            move(position->y, position->x);
            moveFirstFrameRight();
            moveLastFrameRight();
        }
        break;
    case 'D': // arrow left
        if (position->current == head)
            break;
        position->current = position->current->prev;
        if (position->current->next->data == ENTER)
        {
            position->x = current_row_length();
            if (position->current->next == documentInfo->frameFirstNode)
            {
                moveFirstFrameLeft();
                moveLastFrameLeft();
                move(position->y, position->x);
            }
            else
            {
                move(--position->y, position->x);
            }
        }
        else
        {
            move(position->y, --position->x);
        }
        break;
    }
    print();
}

void save(void)
{
    if(fileInfo->isNewFile) {

    }  
    Node *p = head;
    while(p->next == tail) {

    }
}

void commonKey(int key)
{
    move(position->y, ++position->x);
    if (position->x == windowSize->x - 2)
    {
        documentInfo->lineCount++;
        move(++position->y, 1);
        position->x = 1;
        //    enter();
    }
    insert(key);
    print();
}

void readFile(char *filename)
{
    fileInfo->isFileReading = true;
    char *dot = strrchr(filename, '.');
    if (dot)
        fileInfo->filetype = dot + 1;
    fileInfo->filename = filename;
    FILE *pFile = fopen(filename, "r");

    int ch;

    bool flag = true;
    do
    {
        ch = fgetc(pFile);
        if (ch == '\n') {
            enter();
        }
            
        else
            commonKey(ch);

        if (documentInfo->lineCount == windowSize->y && flag)
        {
            Node *p = position->current->prev;
            documentInfo->frameLastNode = p;
            flag = false;
        }
    } while (ch != EOF);

    fclose(pFile);
    move(0, 0);
    position->x = 0;
    position->y = 0;
    documentInfo->frameY = 0;
    position->current = head;
    fileInfo->isFileReading = false;
    print();
}

void ctrl_q(void)
{
    endwin();
    exit(0);
}

struct termios orig_termios;

void disableCtrlFunctions()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON);
    raw.c_lflag &= ~(IEXTEN | ISIG);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main(int argc, char *argv[])
{
    initLinkedList();
    initCurses();
    initWindowSize();
    initDocumentInfo();
    initFileInfo();

    print();
    disableCtrlFunctions();

    if (argv[1] != NULL)
        readFile(argv[1]);

    while (1)
    {
        int key = getch();
        if (key == BACKSPACE)
            backspace();
        else if (key == ENTER)
        {
            enter();
            fileInfo->isUpdated = true;
        }

        else if (key == '\033')
            arrow();
        else if (key == CTRL('f'))
            save();
        else if (key == CTRL('s'))
            save();
        else if (key == CTRL('q'))
            ctrl_q();
        else
        {
            commonKey(key);
            fileInfo->isUpdated = true;
        }
    }
    return 0;
}
