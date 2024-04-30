
# ZSO

Projekt z przedmiotu Zaawansowane Systemy Operacyjne. Jego celem jest zaimplementowanie poprawnej synchronizacji wątków w programie wielowątkowym zgodnie z [treścią](#treść) zadania.

## Treść
Lotnisko, odprawa, prześwietlanie bagażu. Pasażerowie wybierają sobie jedno z trzech przejść z prześwietleniem bagażu. Samo prześwietlanie odbywa się automatycznie i pasażer kładzie walizki na taśmie, taśma się przesuwa, prześwietla i jak detektor nie stwierdzi żadnych problemów to pasażer idzie sobie dalej. Jak detector stwierdzi problem, włącza się syrena i ta bramka zostaje wstrzymana, a pasażer przechodzi do kontroli osobistej, która trwa dość długo. Jak nic groźnego nie znaleziono to pasażer może wejść do samolotu. Jak znaleziono to pasażer jest zatrzymany i do samolotu nie wchodzi. Jeżeli w trakcie kontroli osobistej jednego z pasażerów znowu włączyła się   syrena to wstrzymywane są wszystkie stanowiska i wszyscy czekają na wyjasnienie sytuacji. Po wyjaśnieniu sytuacji (podejrzany pasażer, albo wsiadł samolotu, albo został zatrzymany) stanowiska ruszają.

## Kompilacja
By skompilować program bez wypisywania na stdout:

    gcc -lpthread -o build/lotnisko lotnisko.c

By skompilować program z info do debugowania bez wypisywania na stdout:

    gcc -g -lpthread -o build/lotnisko lotnisko.c


By skompilować program z wypisywaniem na stdout:

    gcc -lpthread -DDEBUG -o build/lotnisko_d lotnisko.c

By skompilować program z info do debugowania i z wypisywaniem na stdout:

    gcc -g -lpthread -DDEBUG -o build/lotnisko_d lotnisko.c

By skompilować program w obu wersjach z info do debugowania:

    gcc -g -lpthread -o build/lotnisko lotnisko.c && gcc -g -lpthread -DDEBUG -o build/lotnisko_d lotnisko.c

Ewentualnie jeszcze z najmniejszym poziomem optymalizacji (by zasymulować pracę przy bramkach):

    gcc -g -O0 -lpthread -o build/lotnisko lotnisko.c && gcc -g -O0 -lpthread -DDEBUG -o build/lotnisko_d lotnisko.c

## Debugowanie
Debugowanie za pomocą:

    valgrind --log-file=log_helgrind.txt --tool=helgrind ./lotnisko
    valgrind --log-file=log_drd.txt --tool=drd ./lotnisko

