#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <algorithm>
#include <Windows.h>
#include <filesystem>
#include <shlobj.h>
#include <string>
#include <mutex>

#include "font_renderer.hpp"
#include "funcs.hpp"
#include "mat_rain.hpp"
#include "classes.hpp"
#include "rand/cs_random.hpp"

#include <SDL2/include/SDL2/SDL.h>
#include <SDL2/include/SDL2/SDL_image.h>

#define WINDOW_WIDTH 600
#define WINDOW_HEIGHT 600
#define MINVAL(a, b) (a < b ? a : b)
#define MAXVAL(a, b) (a > b ? a : b)
#define SET_THIS_VAL 1245937938

void relFiles(std::vector<DisFile>& files, SDL_Renderer* ren, std::string curDir, std::string key){
    files.clear();
    for (EncryptedFl eFl: gAllFiles("data" + curDir, key)){
        files.push_back(DisFile(ren, eFl.name));
    }
}

void relFds(std::vector<DisFolder>& folders, SDL_Renderer* ren, std::string curDir, std::string key){
    folders.clear();
    for (EncryptedFd eFd: gAllFolders("data" + curDir, key)){
        folders.push_back(DisFolder(ren, eFd.name));
    }
}

// threaded store functions
std::mutex mtx;
void tStoFl(std::vector<DisFile>& files, SDL_Renderer* ren, std::string curDir, std::string sFilePath, std::string folder, std::string key, bool& storing){
    mtx.lock();
    storing = true;
    stoFile(sFilePath, folder, key);
    relFiles(files, ren, curDir, key); // i think this is safe
    storing = false;
    mtx.unlock();
}
void tStoFd(std::vector<DisFolder>& folders, SDL_Renderer* ren, std::string curDir, std::string sFdPath, std::string key, bool& storing){
    mtx.lock();
    storing = true;
    stoFd(sFdPath, curDir, key);
    relFds(folders, ren, curDir, key);
    storing = false;
    mtx.unlock();
}

SDL_Rect gRenRect(SDL_Rect org, SDL_Rect mr, SDL_Cursor* cCur){
    SDL_Rect renRect = org;
    if (SDL_HasIntersection(&org, &mr)){
        renRect.x -= 1;
        renRect.y -= 1;
        renRect.w += 2;
        renRect.h += 2;
        SDL_SetCursor(cCur);
    }
    return renRect;
}

int main(int argc, char* argv[]){

    // setup sdl
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    SDL_Window* win = SDL_CreateWindow("RFS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_Surface* icSurf = IMG_Load("images/icon.png");
    SDL_SetWindowIcon(win, icSurf);
    SDL_FreeSurface(icSurf);
    SDL_Cursor* cCur = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    SDL_Cursor* nCur = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);

    // console window
    HWND conHandle = GetConsoleWindow();
    ShowWindow(conHandle, SW_HIDE);

    // setup backend
    if (!setup()) return 0;

    // state
    bool running = true;
    int STATE_MAIN = 1, STATE_LOGIN = 2;
    int state = STATE_LOGIN;
    long long ticks = 0;

    // mouse pos
    int mx, my;
    SDL_Rect mr;

    // fps capping
    const int FPS = 60;
    const float FR_DELAY = 1000 / FPS;
    Uint32 frS = 0;
    int frT = 0;

    // login variables
    std::string longPass(32, '*');
    SDL_Rect pfRect;
    std::string stoPass, salt, inPass, key, lastTyped;
    bool newAcc = !gStoredPass(stoPass, salt);
    bool curBlink = false, pfFocused = true, curIdle = true;
    int curPos = 0, curTicks = 0, curIdleCd = 0;
    std::vector<MatRainStreak> matRains;

    // main variables
    std::string curDir = "";
    std::string curDisDir = "";
    int scrollY = 0, maxSY, scrollSc = 30, lsClickL = 0, dbClickMax = 13, contH, sbHOffset, sbY;
    int stOffset = 10 + textHeight(ren, "L", 20), endOffset = 50;
    std::vector<DisFile> files;
    std::vector<DisFolder> folders;
    SDL_Rect sbRect;
    bool dbClick = false, sbHeld = false, selClick = false, ctrlHeld = false, storing = false;
    std::string dropPath;
    std::string movToFd = "";

    // things to store
    std::vector<std::pair<std::string, std::string>> toStoFls;
    std::vector<std::pair<std::string, std::string>> toStoFds;

    // selected things
    std::vector<std::string> selFls;
    std::vector<std::string> selFds;

    // utility bar things
    SDL_Texture* prevTex = loadTexture(ren, "images/prev.png");
    SDL_Rect prevRect = {10, WINDOW_HEIGHT - endOffset + 10, 31, 31};
    SDL_Texture* flTex = loadTexture(ren, "images/file.png");
    SDL_Rect flRect = {prevRect.x + prevRect.w + 10, WINDOW_HEIGHT - endOffset + 10, 25, 31};
    SDL_Texture* fdTex = loadTexture(ren, "images/folder.png");
    SDL_Rect fdRect = {flRect.x + flRect.w + 10, WINDOW_HEIGHT - endOffset + 10, 36, 31};
    SDL_Texture* pwTex = loadTexture(ren, "images/password.png");
    SDL_Rect pwRect = {fdRect.x + fdRect.w + 10, WINDOW_HEIGHT - endOffset + 10, 25, 31};
    SDL_Texture* binTex = loadTexture(ren, "images/bin.png");
    SDL_Rect binRect = {pwRect.x + pwRect.w + 10, WINDOW_HEIGHT - endOffset + 10, 21, 31};
    SDL_Texture* expTex = loadTexture(ren, "images/export.png");
    SDL_Rect expRect = {binRect.x + binRect.w + 10, WINDOW_HEIGHT - endOffset + 10, 32, 31};

    while (running){

        ticks += 1;
        SDL_GetMouseState(&mx, &my);
        mr = {mx, my, 1, 1};

        if (state == STATE_LOGIN){
            
            // handle events
            curIdle = true;
            lastTyped = "";
            SDL_Event ev;
            while (SDL_PollEvent(&ev) != 0){
                if (ev.type == SDL_QUIT){
                    running = false;
                } else if (ev.type == SDL_TEXTINPUT || ev.type == SDL_KEYDOWN){
                    if (ev.type == SDL_TEXTINPUT){
                        if (inPass.size() < 32 && pfFocused){
                            std::string c = ev.text.text;
                            inPass.insert(curPos, c);
                            curPos += 1;
                            curIdle = false;
                            lastTyped = c;
                        }
                    } else {
                        SDL_Keycode inkey = ev.key.keysym.sym;
                        if (inkey == SDLK_BACKSPACE && inPass.size() && curPos){
                            inPass = inPass.substr(0, curPos - 1) + inPass.substr(curPos, inPass.size());
                            curPos -= 1;
                            curIdle = false;
                        } else if (inkey == SDLK_RIGHT){
                            curPos = MINVAL(int(inPass.size()), curPos + 1);
                            curIdle = false;
                        } else if (inkey == SDLK_LEFT){
                            curPos = MAXVAL(0, curPos - 1);
                            curIdle = false;
                        } else if (inkey == SDLK_RETURN && pfFocused){
                            if (newAcc){
                                stoNewPass(inPass);
                                state = STATE_MAIN;
                                gStoredKey(key, inPass);
                                relFiles(files, ren, curDir, key);
                                relFds(folders, ren, curDir, key);
                            } else {
                                if (sha256(inPass + salt, 100000) == stoPass){
                                    state = STATE_MAIN;
                                    gStoredKey(key, inPass);
                                    relFiles(files, ren, curDir, key);
                                    relFds(folders, ren, curDir, key);
                                } else {
                                    inPass = "";
                                    curPos = inPass.size();
                                }
                            }
                        }
                    }
                } else if (ev.type == SDL_MOUSEBUTTONDOWN){
                    if (SDL_HasIntersection(&mr, &pfRect)){
                        pfFocused = true;
                    } else {
                        pfFocused = false;
                    }
                }
            }

            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);

            // matrix rain
            matRains.erase(std::remove_if(
                matRains.begin(), matRains.end(),
                [&ren](MatRainStreak& mrs){return mrs.update(ren);}
            ), matRains.end());
            if (ticks % 10 == 0) matRains.push_back(MatRainStreak(randInt(0, 70) * 10));
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 128);
            SDL_Rect fillBg = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
            SDL_RenderFillRect(ren, &fillBg);
            
            // title text
            renderText(
                ren,
                Color(0, 255, 0), "RFS",
                (WINDOW_WIDTH - textWidth(ren, "RFS", 50)) / 2, 100,
                50
            );

            // caption
            renderText(
                ren,
                Color(0, 255, 0), "Login Page",
                (WINDOW_WIDTH - textWidth(ren, "Login Page", 20)) / 2, 170,
                20
            );

            // password field title
            std::string pfTitle = newAcc ? "Enter a password..." : "Enter your password...";
            renderText(
                ren,
                Color(0, 255, 0), pfTitle,
                (WINDOW_WIDTH - textWidth(ren, pfTitle, 18)) / 2, 310,
                18
            );

            // password field
            pfRect = {int((WINDOW_WIDTH - (textWidth(ren, longPass, 17) + 20)) / 2), 350, int(textWidth(ren, longPass, 17) + 20), 40};
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderFillRect(ren, &pfRect);
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            SDL_RenderDrawRect(ren, &pfRect);
            std::string hidPass(inPass.size(), '*');
            renderText(
                ren,
                Color(0, 255, 0), hidPass,
                pfRect.x + 10, pfRect.y + 8,
                17
            );
            SDL_Rect curRect = {pfRect.x + 10 + int(textWidth(ren, hidPass.substr(0, curPos), 17)), pfRect.y + 10, 3, int(textHeight(ren, "L", 17))};
            if (pfFocused && curBlink) SDL_RenderFillRect(ren, &curRect);
            if (curTicks % 30 == 0) curBlink = !curBlink;
            if (!curIdle){
                curIdleCd = 30; curBlink = true;
            }
            curIdleCd -= 1;
            if (curIdleCd <= 0) curTicks += 1;

            SDL_RenderPresent(ren);

        } else if (state == STATE_MAIN){

            // clear folders and files to be stored
            toStoFds.clear();
            toStoFls.clear();

            // set cursor
            SDL_SetCursor(nCur);

            // key holds
            ctrlHeld = false;
            const Uint8* kbState = SDL_GetKeyboardState(NULL);
            if (kbState[SDL_SCANCODE_LCTRL] || kbState[SDL_SCANCODE_RCTRL]){
                ctrlHeld = true;
            }
            
            // handle events
            SDL_Event ev;
            dbClick = false;
            selClick = false;
            while (SDL_PollEvent(&ev) != 0){
                if (ev.type == SDL_QUIT){
                    running = false;
                } else if (ev.type == SDL_KEYDOWN){
                    // nothing
                } else if (ev.type == SDL_MOUSEWHEEL){
                    if (ev.wheel.y < 0){ // scrolling up
                        if ((stOffset + 20 + maxSY) > WINDOW_HEIGHT){
                            scrollY = MAXVAL(-(maxSY - WINDOW_HEIGHT), scrollY - scrollSc);
                        };
                    } else if (ev.wheel.y > 0){ // scrolling down
                        scrollY = MINVAL(0, scrollY + scrollSc);
                    }
                } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT){
                    // double click
                    if (ticks - lsClickL <= dbClickMax) dbClick = true;
                    lsClickL = ticks;
                    // change password
                    if (SDL_HasIntersection(&pwRect, &mr)){
                        ShowWindow(conHandle, SW_SHOW);
                        system("cls");
                        std::string newPass;
                        std::cout << "Welcome to the password changing menu!\n";
                        while (true){
                            std::cout << "Enter your new password (enter !EXIT to exit without changing your password): ";
                            std::getline(std::cin, newPass);
                            if (newPass != "!EXIT"){
                                if (newPass.size() < 32){
                                    reKey(newPass, inPass);
                                    stoNewPass(newPass);
                                    gStoredKey(key, newPass);
                                    break;
                                } else {
                                    std::cout << "Password must be under 32 characters long! Try again.\n";
                                }
                            } else {break;}
                        }
                        ShowWindow(conHandle, SW_HIDE);
                    }
                    // scrollbar held
                    if (SDL_HasIntersection(&sbRect, &mr)){
                        sbHeld = true;
                        sbHOffset = my - sbY;
                    }
                    // move out of current folder
                    if (SDL_HasIntersection(&prevRect, &mr)){
                        movFd("$OUT", curDir, curDisDir, key);
                        relFds(folders, ren, curDir, key);
                        relFiles(files, ren, curDir, key);
                        selFls.clear();
                        selFds.clear();
                        scrollY = 0;
                        sbHeld = false;
                        scrollY = 0;
                    }
                    // browse for file to store
                    if (SDL_HasIntersection(&flRect, &mr)){
                        OPENFILENAME ofObj;
                        ZeroMemory(&ofObj, sizeof(ofObj));
                        char tmp[MAX_PATH];
                        ofObj.lpstrFile = tmp;
                        ofObj.lpstrFile[0] = '\0';
                        ofObj.lpstrFilter = NULL;
                        ofObj.lStructSize = sizeof(OPENFILENAMEA);
                        ofObj.nFileOffset = 0;
                        ofObj.Flags = OFN_PATHMUSTEXIST;
                        ofObj.lpstrTitle = "Add file to vault";
                        ofObj.nMaxFile = MAX_PATH;
                        ofObj.hwndOwner = NULL;
                        std::string bfCwd = std::filesystem::current_path().string();
                        if (GetOpenFileName(&ofObj)){
                            toStoFls.push_back(std::make_pair(std::string(ofObj.lpstrFile), curDisDir));
                        }
                        std::filesystem::current_path(bfCwd); // why does GetOpenFileName change the cwd? bruh
                    }
                    // browse for folder to store
                    if (SDL_HasIntersection(&fdRect, &mr)){
                        BROWSEINFOA biObj;
                        char fdDir[MAX_PATH];
                        biObj.hwndOwner = NULL;
                        biObj.lpszTitle = "Add folder to vault";
                        biObj.pszDisplayName = fdDir;
                        biObj.pidlRoot = NULL;
                        biObj.ulFlags = BIF_USENEWUI;
                        biObj.lParam = 0;
                        LPITEMIDLIST itmList = SHBrowseForFolder(&biObj);
                        if (itmList != NULL){
                            SHGetPathFromIDList(itmList, fdDir);
                            std::string sFdPath(fdDir);
                            toStoFds.push_back(std::make_pair(sFdPath, curDisDir));
                        }
                    }
                    // delete folders and files
                    if (SDL_HasIntersection(&binRect, &mr)){
                        for (std::string flName: selFls){
                            delFl(flName, curDir, key);
                        }
                        for (std::string fdName: selFds){
                            delFd(fdName, curDir, key);
                        }
                        selFls.clear();
                        selFds.clear();
                        relFiles(files, ren, curDir, key);
                        relFds(folders, ren, curDir, key);
                    }
                    // export to folder
                    if (SDL_HasIntersection(&expRect, &mr)){
                        BROWSEINFOA biObj;
                        char fdDir[10000];
                        biObj.hwndOwner = NULL;
                        biObj.lpszTitle = "Add folder to vault";
                        biObj.pszDisplayName = fdDir;
                        biObj.pidlRoot = NULL;
                        biObj.ulFlags = BIF_USENEWUI;
                        biObj.lpfn = NULL;
                        biObj.lParam = 0;
                        LPITEMIDLIST itmList = SHBrowseForFolder(&biObj);
                        if (itmList != NULL){
                            SHGetPathFromIDList(itmList, fdDir);
                            std::string expPath(fdDir);
                            for (std::string flName: selFls){
                                std::thread nThread(exportFl, flName, curDir, expPath, key);
                                nThread.detach();
                            }
                            for (std::string fdName: selFds){
                                std::thread nThread(exportFd, fdName, curDir, expPath, key);
                                nThread.detach();
                            }
                            selFls.clear();
                            selFds.clear();
                        }
                    }
                    selClick = true;
                } else if (ev.type == SDL_MOUSEBUTTONUP){
                    sbHeld = false;
                } else if (ev.type == SDL_DROPFILE){
                    dropPath = ev.drop.file;
                    // store file or folder
                    if (std::filesystem::is_directory(dropPath)){
                        toStoFds.push_back(std::make_pair(dropPath, curDisDir));
                    } else {
                        toStoFls.push_back(std::make_pair(dropPath, curDisDir));
                    }
                }
            }

            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);

            // render and update files and folders
            int renY = stOffset + 20 + scrollY;
            int actHeight = WINDOW_HEIGHT - stOffset - endOffset;
            contH = 0;
            SDL_Rect hlRect;
            bool collAny = false;
            for (DisFolder& fd: folders){
                if (fd.collides(mr, renY, hlRect)){
                    collAny = true;
                    SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                    SDL_RenderFillRect(ren, &hlRect);

                    // open folder
                    if (dbClick){
                        movToFd = fd.name;
                    }

                    // selections
                    if (selClick){
                        if (!ctrlHeld){
                            selFds.clear();
                            selFls.clear();
                        }

                        // sorry
                        if (std::find(selFds.begin(), selFds.end(), fd.name) != selFds.end()){
                            selFds.erase(std::find(selFds.begin(), selFds.end(), fd.name));
                        } else {
                            selFds.push_back(fd.name);
                        }
                    }
                }

                // render background if selected
                if (std::find(selFds.begin(), selFds.end(), fd.name) != selFds.end()){
                    SDL_SetRenderDrawColor(ren, 70, 70, 70, 255);
                    SDL_RenderFillRect(ren, &hlRect);
                }

                fd.render(ren, renY);
                renY += fd.rect.h + 15;
                contH += fd.rect.h + 15;
            }
            for (DisFile& fl: files){
                if (fl.collides(mr, renY, hlRect)){
                    collAny = true;
                    SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                    SDL_RenderFillRect(ren, &hlRect);

                    // open file
                    if (dbClick){
                        std::thread nThread(oFile, fl.name, curDir, key, true, "temp");
                        nThread.detach();
                    }

                    // selections
                    if (selClick){
                        if (!ctrlHeld){
                            selFds.clear();
                            selFls.clear();
                        }

                        // not again
                        if (std::find(selFls.begin(), selFls.end(), fl.name) != selFls.end()){
                            selFls.erase(std::find(selFls.begin(), selFls.end(), fl.name));
                        } else {
                            selFls.push_back(fl.name);
                        }
                    }
                }

                // render background if selected
                if (std::find(selFls.begin(), selFls.end(), fl.name) != selFls.end()){
                    SDL_SetRenderDrawColor(ren, 70, 70, 70, 255);
                    SDL_RenderFillRect(ren, &hlRect);
                }

                fl.render(ren, renY);
                renY += fl.rect.h + 15;
                contH += fl.rect.h + 15;
            }
            maxSY = renY - scrollY + endOffset;

            // clear selections if not collided with anything
            if (!collAny && !ctrlHeld && selClick){
                selFls.clear();
                selFds.clear();
            }

            // display current folder
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_Rect cfBgRect = {0, 0, WINDOW_WIDTH, stOffset};
            SDL_RenderFillRect(ren, &cfBgRect);
            renderText(
                ren, Color(0, 255, 0),
                "Current Folder: " + (curDisDir.size() ? curDisDir : "/"),
                10, 10, 20
            );
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            SDL_RenderDrawLine(ren, 0, stOffset, WINDOW_WIDTH, stOffset);

            // scrolling
            if (sbHeld){
                int cMY = MAXVAL(my, 36) - 36; // clamp mouse y
                scrollY = -(float(cMY) / actHeight * contH) + (float(sbHOffset) / actHeight * contH);

                // clamp scrolly
                scrollY = MAXVAL(-(maxSY - WINDOW_HEIGHT), scrollY);
                scrollY = MINVAL(0, scrollY);
            }

            // scrollbar
            if (contH > WINDOW_HEIGHT){
                sbRect = {
                    WINDOW_WIDTH - 20, int(float(-scrollY) / contH * actHeight),
                    20, MAXVAL(int(float(actHeight) / contH * actHeight), 5)
                };
                sbRect.y += stOffset + 2;
                sbY = sbRect.y - 1; // offset of 1 for some reason
                SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
                SDL_RenderFillRect(ren, &sbRect);
            }

            // utility bar
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_Rect uBarRect = {0, WINDOW_HEIGHT - endOffset, WINDOW_WIDTH, endOffset};
            SDL_RenderFillRect(ren, &uBarRect);
            SDL_SetRenderDrawColor(ren, 0, 255, 0, 255);
            SDL_RenderDrawLine(ren, 0, WINDOW_HEIGHT - endOffset, WINDOW_WIDTH, WINDOW_HEIGHT - endOffset);

            SDL_Rect prevRenRect = gRenRect(prevRect, mr, cCur);
            SDL_RenderCopy(ren, prevTex, NULL, &prevRenRect);
            SDL_Rect flRenRect = gRenRect(flRect, mr, cCur);
            SDL_RenderCopy(ren, flTex, NULL, &flRenRect);
            SDL_Rect fdRenRect = gRenRect(fdRect, mr, cCur);
            SDL_RenderCopy(ren, fdTex, NULL, &fdRenRect);
            SDL_Rect pwRenRect = gRenRect(pwRect, mr, cCur);
            SDL_RenderCopy(ren, pwTex, NULL, &pwRenRect);
            SDL_Rect binRenRect = gRenRect(binRect, mr, cCur);
            if (selFls.size() || selFds.size()) SDL_RenderCopy(ren, binTex, NULL, &binRenRect);
            SDL_Rect expRenRect = gRenRect(expRect, mr, cCur);
            if (selFls.size() || selFds.size()) SDL_RenderCopy(ren, expTex, NULL, &expRenRect);

            // render storing message
            if (storing){
                renderText(
                    ren, Color(0, 255, 0),
                    "Storing...",
                    WINDOW_WIDTH - 10 - textWidth(ren, "Storing...", 20), WINDOW_HEIGHT - endOffset + 10,
                    20
                );
            }

            SDL_RenderPresent(ren);

            // move folder
            if (movToFd.size()){
                movFd(movToFd, curDir, curDisDir, key);
                relFiles(files, ren, curDir, key);
                relFds(folders, ren, curDir, key);
                selFls.clear();
                selFds.clear();
                scrollY = 0;
                sbHeld = false;
                movToFd = "";
            }

            // store queued files and folders
            for (auto [sFilePath, folder]: toStoFls){
                std::thread nThread(tStoFl, std::ref(files), std::ref(ren), curDir, sFilePath, folder, key, std::ref(storing));
                nThread.detach();
            }
            for (auto [sFdPath, folder]: toStoFds){
                std::thread nThread(tStoFd, std::ref(folders), std::ref(ren), curDir, sFdPath, key, std::ref(storing));
                nThread.detach();
            }
            if (toStoFls.size()) relFiles(files, ren, curDir, key);
            if (toStoFds.size()) relFds(folders, ren, curDir, key);

        }

        // cap fps
        frT = SDL_GetTicks() - frS;
        if (FR_DELAY > frT) SDL_Delay(FR_DELAY - frT);
        frS = SDL_GetTicks();

    }

    SDL_Quit();
    IMG_Quit();
    TTF_Quit();
    return 0;

}