## Tarjeta

<!-- Una tarjeta de Trello por PR. Enlazala y escribe su ID. -->

- Tarjeta:
- Responsable:

## Que cambia

<!-- Una o dos frases. Si el diff no cabe aqui, el PR es demasiado grande. -->

## Como se verifico

- [ ] `cmake --build build` sin warnings en `include/` ni en `src/`
- [ ] `ctest --test-dir build --output-on-failure` verde
- [ ] `bash scripts/check_domain.sh` OK (solo si toca `include/` o `src/`)
- [ ] ASan/UBSan limpio (solo si toca memoria)
- [ ] `docs/SDD.md` actualizado (solo si cambio una firma publica)

## Riesgos y notas

<!-- Que puede romper, que queda pendiente y que se decidio. -->

## Para el revisor

<!-- Que mirar primero y que NO hace falta revisar. -->
