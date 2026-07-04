# deecer-tui [WORK IN PROGRESS]
Un cliente de consola para deezer, escrito en C y usando ncurses, libcurl, cjson y libmpv.

## Introducción
Este es un proyecto con la única intención de aprender el lenguaje C, por lo que muy probablemente te encontrarás con fallos gordos de novato. En los días en los que la IA es capaz de hacerlo todo por tí, yo, un mal programador de lenguajes de alto nivel, he decido volver a los orígenes y comenzar este proyecto para aprender el lenguaje de programación de los señores mayores cómo yo.  
Una de las cosas que puede que te llamen la atención es que este repo esté documentado en español, y si miras el código, la mayoría de comentarios también están en español. Esto tiene una explicación, yo hablo español xD

## Funcionalidades actuales
- Se pueden hacer búsquedas sencillas a la api pública de deezer y nos devuelve una lista de tracks
- Se puede reproducir el preview de los tracks de la lista devuelta por la busqueda
- La reproduccion no para la ui, funciona en un hilo separado


## Funcionalidades previstas
- Poder reproducir las canciones completas
- Mostrar listas de reproduccion del usuario
- Poder reproducir una lista de tracks continua
- Completar funciones del menu (Home/Explore/Library/Settings)

## Notas
Para aprender he ido combinando lecturas clásicas cómo el libro `El lenguaje de programacion C` de K&R, visionado de videos de Youtube, lecturas de `man <LIBRARY_FUNCTION>` (¡algo que me ha encantado!), posts de StackOverflow, Reddit y demás foros de internet, y sí, también he preguntado en ocasiones a la IA. He usado la web de deepseek para hacerle preguntas de las que no conseguía respuesta buscando de otros modos o necesitaba respuesta a un tema demasiado concreto, pero siempre intentando que me explicase la respuesta como si fuera un profe, y no que me diera el código con la solución. En otras ocasiones le hubiera pedido trozos de código, pero en este proyecto prefería ir descubriendo yo las soluciones.

## Enlaces
[El lenguaje de programacion C](https://frrq.cvg.utn.edu.ar/pluginfile.php/13741/mod_resource/content/0/El-lenguaje-de-programacion-C-2-ed-kernighan-amp-ritchie.pdf) por Kernighan y Ritchie

[Un curso de C de 7 horas en youtube gratis](https://www.youtube.com/watch?v=xND0t1pr3KY&list=PL2v1b_nyIe6Wyt8E1pJ_kuYp5Q0JTZ6ak&index=4) por Bro Code  

[Uno de los creadores de libcurl enseñandote a usarlo](https://www.youtube.com/watch?v=nbTaHEocCuo) por Daniel Stenberg  

[Tutorial cortito para usar la libreria cjson](https://www.youtube.com/watch?v=BR5TlKwpr9M) por forked  

[Un cliente deezer de consola hecho en rust](https://github.com/Minuga-RC/deezer-tui) por Minuga-RC

[Memoria dinamica en C] (https://www.youtube.com/playlist?list=PLja70d5XIV8aF_PpHS2dOrpuiSjnkGTsX) por New line

