
[Polski](#zso-projekt-pl)

# ZSO (Advanced Operating Systems) Project \[en\]

This is a project for the Advanced Operating Systems course. Its goal is to 
implement correct thread synchronization in a multi-threaded program according 
to the [project description](#description).

## Description
Airport, check-in, baggage screening. Passengers choose one of three baggage 
screening gates. The screening process is automated, and the passenger places 
their luggage on the conveyor belt, which then moves forward, scans the 
luggage, and if the detector does not identify any issues, the passenger 
proceeds. If the detector identifies a problem, an alarm is triggered, 
and that gate is halted, while the passenger undergoes a personal check, 
which takes a considerable amount of time. If nothing dangerous is found, 
the passenger may board the plane. If something dangerous is detected, the 
passenger is detained and denied boarding. If, during a personal check of one 
passenger, the alarm is triggered again, all gates are halted, and everyone 
waits for the situation to be resolved. Once the situation is clarified 
(whether the suspicious passenger is allowed to board or is detained), 
the stations resume operation.

## Compilation
To compile the program with debugging info without printing to stdout:

    gcc -g -lpthread -o lotnisko lotnisko.c

To compile the program with stdout output:

    gcc -lpthread -DDEBUG -o lotnisko_d lotnisko.c

To compile the program in both versions:

    gcc -g -lpthread -o lotnisko lotnisko.c && gcc -lpthread -DDEBUG -o lotnisko_d lotnisko.c

## Debugging
To debug the program I used the following commands after compiling with the 
aforementioned `-g` option:

    valgrind --log-file=log_helgrind.txt --tool=helgrind ./lotnisko
    valgrind --log-file=log_drd.txt --tool=drd ./lotnisko


# ZSO Projekt \[pl\]

Projekt z przedmiotu Zaawansowane Systemy Operacyjne. Jego celem jest 
zaimplementowanie poprawnej synchronizacji wątków w programie wielowątkowym 
zgodnie z [treścią](#treść) zadania.

## Treść
Lotnisko, odprawa, prześwietlanie bagażu. Pasażerowie wybierają sobie jedno z 
trzech przejść z prześwietleniem bagażu. Samo prześwietlanie odbywa się 
automatycznie i pasażer kładzie walizki na taśmie, taśma się przesuwa, 
prześwietla i jak detektor nie stwierdzi żadnych problemów to pasażer idzie 
sobie dalej. Jak detector stwierdzi problem, włącza się syrena i ta bramka 
zostaje wstrzymana, a pasażer przechodzi do kontroli osobistej, która trwa 
dość długo. Jak nic groźnego nie znaleziono to pasażer może wejść do samolotu. 
Jak znaleziono to pasażer jest zatrzymany i do samolotu nie wchodzi. Jeżeli w 
trakcie kontroli osobistej jednego z pasażerów znowu włączyła się syrena to 
wstrzymywane są wszystkie stanowiska i wszyscy czekają na wyjaśnienie 
sytuacji. Po wyjaśnieniu sytuacji (podejrzany pasażer, albo wsiadł samolotu, 
albo został zatrzymany) stanowiska ruszają.

## Kompilacja
By skompilować program z info do debugowania bez wypisywania na stdout:

    gcc -g -lpthread -o lotnisko lotnisko.c

By skompilować program z wypisywaniem na stdout:

    gcc -lpthread -DDEBUG -o lotnisko_d lotnisko.c

By skompilować program w obu wersjach:

    gcc -g -lpthread -o lotnisko lotnisko.c && gcc -lpthread -DDEBUG -o lotnisko_d lotnisko.c

## Debugowanie
Do debugowania programu używałem poniższych komend po skompilowaniu ze 
wspomnianą wyżej opcją `-g`:

    valgrind --log-file=log_helgrind.txt --tool=helgrind ./lotnisko
    valgrind --log-file=log_drd.txt --tool=drd ./lotnisko

