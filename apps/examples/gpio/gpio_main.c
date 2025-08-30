/****************************************************************************
 * apps/examples/gpio/gpio_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include <nuttx/ioexpander/gpio.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int fd;
  int ret;
  bool value;

  if (argc < 2)
    {
      /* Se a quantidade de argumentos for menor que 2, mostra o uso */

      fprintf(stderr, "Uso: %s <devpath> [<value>]\n", argv[0]);
      return 1;
    }

  /* Tenta abrir o dispositivo com permissão de leitura e escrita */

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "Erro ao abrir %s: %d\n", argv[1], fd);
      return 1;
    }

  /* Se o número de argumentos for 3, assuma que é uma operação de escrita */

  if (argc == 3)
    {
      /* Converte a string do argumento para um valor booleano (0 ou 1) */

      value = atoi(argv[2]) > 0;
      
      /* Usa o comando GPIOC_WRITE do ioctl para escrever o valor no pino */

      ret = ioctl(fd, GPIOC_WRITE, (unsigned long)&value);
    }
  else
    {
      /* Se o número de argumentos for 2, assuma que é uma operação de leitura */
      
      ret = ioctl(fd, GPIOC_READ, (unsigned long)&value);
    }

  /* Fecha o descritor do arquivo */
  
  close(fd);

  if (ret < 0)
    {
      fprintf(stderr, "Erro na operação: %d\n", ret);
      return 1;
    }
  else
    {
      if (argc == 2)
        {
          /* Se for uma leitura, imprime o valor lido */
          
          fprintf(stdout, "Valor lido: %d\n", value);
        }
      return 0;
    }
}

