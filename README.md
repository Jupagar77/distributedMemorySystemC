Correr programas:
1. Agregue codigo en los archivos servidor.c o cliente.c
2. Guarde el archivo.
3. Desde la consola, corra el comando:

    gcc -pthread -o servidor servidor.c

    gcc -o cliente cliente.c

   Nota: -pthread en caso de que el archivo tenga implementacion de hilos.

4. Corra ambos programas, ya sea en:

   Misma computadora:

     ./servidor 9000 #en una consola

     ./cliente localhost 9000 #en otra consola

   Diferentes computadoras:

     ./servidor 9000 #en una computadora

     // busque la ip publica de la computadora servidor (https://www.whatismyip.com), ejemplo 192.28.0.2

     ./cliente 192.28.0.2 9000 #en otra computadora

   Notas: se pueden correr cuantos clientes se deseen y el puerto puede ser el que guste.

Nota: ignorar archivo thread.c
