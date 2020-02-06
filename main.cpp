#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <fstream>

typedef struct thread_data { //Структура данных для хранения информации потока
	double *y_total;//значение части интеграла
	pthread_mutex_t *y_total_lock;//мьютекс
	int threadID;//Номер потока
	double step; //Шаг вычислений
	double start, end;//начальное и конечное интервала интегрирования
} thread_data;

void *rectangle(void *threadD);
double f( double );              // Интегрируемая функция
double dabs( double ); // Абсолютная величина для вещественных чисел

int main(int argc, char const *argv[])
{

	const double REAL  = 2.714242228183835;// Точное значение интеграла 1 /√1 + x^4
        unsigned int NUM_THREADS = 2; // Количество потоков.
	const double a = -2.0; // Левая граница интервала интегрирования.
	const double b =  2.0; // Правая граница интервала интегрирования.
	clock_t     t1;      // Время начала вычислений.
        clock_t     t2;      // Время окончания вычислений.
	double secs; // Время вычислений в секундах.

	int i;//Счетчик цикла
	double Ivalue, //Значение интеграла
	       y_total = 0.0;//Сумма от всех потоков
	static long num_steps;// количество интервалов разбиения отрезка интегрирования
	double thread_interval;
	double step;//Шаг интегрирования
	if(argc == 3) {//Если вызов с двумя параметрами, то читаем их
		num_steps = atoi(argv[1]);
		NUM_THREADS=atoi(argv[2]);
	}
	else {//Иначе значения по умолчанию
		num_steps = 1000000;
		NUM_THREADS=2;
	}
	//Расчет шага интегрирования
	step = (double)(b-a) / num_steps;
	thread_interval=(double)(b-a)/NUM_THREADS;

	pthread_t *threads;
	pthread_mutex_t y_total_lock;
	//Критическая секция это запись в переменную y_total
	pthread_mutex_init(&y_total_lock, NULL);
	thread_data *threadD;
	//Массив потоков
	threads = (pthread_t*)malloc(NUM_THREADS * sizeof(pthread_t));
	//Массив данных для потоков
	threadD = (thread_data*)malloc(NUM_THREADS * sizeof(thread_data));
	//Старт вычислений
    t1 = clock( );

	for(i = 0; i < NUM_THREADS; i++) {
		//Записываем данные для передачи потоку
		threadD[i].y_total = &y_total;
		threadD[i].y_total_lock = &y_total_lock;
		threadD[i].threadID = i;
		threadD[i].step = step;
		threadD[i].start= a+i*thread_interval;
		threadD[i].end = a+(i+1)*thread_interval;
		//Создаем поток
		pthread_create(&threads[i], NULL, rectangle, &threadD[i]);
	}
	//Читаем данные от всех потоков
	for(i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	//Завершение вычислений
	t2 = clock(  );
        secs = ( ( double ) ( t2 - t1 ) ) / CLOCKS_PER_SEC;
	//Расчет интеграла
	Ivalue = y_total * step;
	//Удаление мьютекса
	pthread_mutex_destroy(&y_total_lock);


  // Вывод результатов
  printf( "Расчетное значение: интеграл 1 / sqrt(1 + pow(x,4) от %.2f до %.2f: %.15lf\n", a, b, Ivalue );
  printf( "Точное значение: интеграл    1 / sqrt(1 + pow(x,4) от  %.2f до %.2f: %.15lf\n", a, b, REAL );
  printf( "Абсолютная погрешность: %.15lf\n", dabs( REAL - Ivalue ));
  printf( "Относительная погрешность: %.15lf\n", 100*dabs( REAL - Ivalue ) / dabs( REAL ) );
  printf( "Использовано %ld шагов %d потоков.\n", num_steps, NUM_THREADS );
  printf( "Время работы в тиках процессора %ld\n", t2 - t1 );
  printf( "Время работы в секундах %f\n", secs );
  printf( "Всего %ld тиков в секунду.\n", CLOCKS_PER_SEC );

}

// Интегрируемая функция.
inline double f( double x ) {
    return 1 / sqrt(1 + pow(x,4)); //double sqrt(double x);
}

// Абсолютная величина.
inline double dabs( double x ) {
  if (x<0.0){ return -x;}
  else {return x;}
}

//Метод прямоугольников
void *rectangle(void *threadD)
{
	double x, y_partial;
	//Читаем входные данные
	thread_data * tData = (thread_data*)threadD;
	//Вычисляем сумму значений интегрируемой функции
	for(x = tData->start; x < tData->end-tData->step; x=x+tData->step) {
		y_partial += f(x-tData->step + tData->step/2);
	}
	//Критическая секция Запись в глобальную переменную
	pthread_mutex_lock(tData->y_total_lock);
	*tData->y_total += y_partial;
	pthread_mutex_unlock(tData->y_total_lock);
	//Завершение потока
	pthread_exit(NULL);
}