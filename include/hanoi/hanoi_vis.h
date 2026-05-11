#ifndef HANOI_VIS_H
#define HANOI_VIS_H

#include "hanoi/hanoi.h"
#include <fstream>
#include <cstdlib>

// Генерирует HTML файл с визуализацией Ханойской башни и открывает в браузере
static void generate_hanoi_html(MutableSegmentedDeque<HanoiMove>& moves,
                                MutableSegmentedDeque<Ring>& initial_rings,
                                int start_stick) {

    std::ofstream file("hanoi.html");

    // --- Начало HTML ---
    file << "<!DOCTYPE html><html><head><meta charset='utf-8'>" << std::endl;
    file << "<title>Hanoi Tower</title>" << std::endl;
    file << "<style>" << std::endl;
    file << "body { font-family: Arial; background: #1a1a2e; color: #eee; text-align: center; margin: 20px; }" << std::endl;
    file << "h1 { color: #e94560; }" << std::endl;
    file << ".sticks { display: flex; justify-content: center; gap: 60px; margin: 40px 0; }" << std::endl;
    file << ".stick-container { width: 200px; }" << std::endl;
    file << ".stick-label { font-size: 18px; font-weight: bold; margin-bottom: 10px; }" << std::endl;
    file << ".stick-area { position: relative; height: 300px; border-bottom: 4px solid #e94560; display: flex; flex-direction: column; justify-content: flex-end; align-items: center; }" << std::endl;
    file << ".pole { position: absolute; bottom: 0; width: 6px; height: 280px; background: #e94560; border-radius: 3px; }" << std::endl;
    file << ".ring { height: 28px; border-radius: 14px; display: flex; align-items: center; justify-content: center; color: #fff; font-size: 12px; font-weight: bold; margin-bottom: 2px; z-index: 1; transition: all 0.3s ease; }" << std::endl;
    file << ".controls { margin: 20px; }" << std::endl;
    file << "button { background: #e94560; color: #fff; border: none; padding: 12px 24px; font-size: 16px; border-radius: 8px; cursor: pointer; margin: 0 8px; }" << std::endl;
    file << "button:hover { background: #c73650; }" << std::endl;
    file << "button:disabled { background: #555; cursor: default; }" << std::endl;
    file << ".step-info { font-size: 18px; margin: 15px 0; min-height: 30px; color: #0f3460; background: #e2e2e2; display: inline-block; padding: 8px 20px; border-radius: 8px; }" << std::endl;
    file << ".step-counter { font-size: 14px; color: #aaa; margin-top: 5px; }" << std::endl;
    file << "</style></head><body>" << std::endl;

    file << "<h1>Hanoi Tower</h1>" << std::endl;

    // --- Стержни ---
    file << "<div class='sticks'>" << std::endl;
    for (int s = 0; s < 3; s++) {
        file << "<div class='stick-container'>" << std::endl;
        file << "<div class='stick-label'>Stick " << s << "</div>" << std::endl;
        file << "<div class='stick-area' id='stick" << s << "'>" << std::endl;
        file << "<div class='pole'></div>" << std::endl;
        file << "</div></div>" << std::endl;
    }
    file << "</div>" << std::endl;

    // --- Контролы ---
    file << "<div class='step-info' id='info'>Press Start or Next Step</div>" << std::endl;
    file << "<div class='step-counter' id='counter'></div>" << std::endl;
    file << "<div class='controls'>" << std::endl;
    file << "<button onclick='prevStep()' id='prevBtn' disabled>&lt; Prev</button>" << std::endl;
    file << "<button onclick='nextStep()' id='nextBtn'>Next Step &gt;</button>" << std::endl;
    file << "<button onclick='autoPlay()' id='autoBtn'>Auto Play</button>" << std::endl;
    file << "<button onclick='resetAll()'>Reset</button>" << std::endl;
    file << "</div>" << std::endl;

    // --- JavaScript: данные ---
    file << "<script>" << std::endl;

    // Записываем начальное состояние колец
    file << "var initialRings = [";
    for (int i = 0; i < initial_rings.get_count(); i++) {
        if (i > 0) file << ",";
        file << "{size:" << initial_rings.get(i).get_size()
             << ",color:'" << initial_rings.get(i).get_color()
             << "'}";
    }
    file << "];" << std::endl;

    file << "var startStick = " << start_stick << ";" << std::endl;
    file << "var maxSize = 0;" << std::endl;
    file << "for(var i=0;i<initialRings.length;i++) if(initialRings[i].size>maxSize) maxSize=initialRings[i].size;" << std::endl;

    // Записываем массив ходов
    file << "var moves = [";
    for (int i = 0; i < moves.get_count(); i++) {
        if (i > 0) file << ",";
        file << "{from:" << moves.get(i).from_stick_index
             << ",to:" << moves.get(i).to_stick_index
             << ",size:" << moves.get(i).ring_size
             << ",color:'" << moves.get(i).ring_color << "'}";
    }
    file << "];" << std::endl;

    // --- JavaScript: логика анимации ---
    file << R"JS(
var sticks = [[],[],[]];
var currentStep = -1;
var autoInterval = null;

// Сортируем кольца по убыванию размера — большие первыми (они будут внизу)
initialRings.sort(function(a,b){ return b.size - a.size; });

// Кладём все кольца на начальный стержень
for(var i=0; i<initialRings.length; i++) {
    sticks[startStick].push(initialRings[i]);
}

function ringWidth(size) {
    var minW = 40, maxW = 180;       
    if(maxSize <= 1) return maxW;
    
    // Линейная интерполяция: size=1 → 40px, size=maxSize → 180px
    // Чем больше кольцо, тем шире
    return minW + (size - 1) * (maxW - minW) / (maxSize - 1);
}

function render() {
    for(var s=0; s<3; s++) {
        var el = document.getElementById('stick' + s);  // находим div стержня
        
        // Удаляем все кольца, оставляем только pole
        var rings = el.querySelectorAll('.ring');
        for(var r=0; r<rings.length; r++) rings[r].remove();
        
        // sticks[s][0] = нижнее кольцо, sticks[s][last] = верхнее
        // Рисуем с последнего к первому, чтобы нижнее оказалось внизу
        for(var i=sticks[s].length-1; i>=0; i--) {
            var ring = sticks[s][i];
            var div = document.createElement('div'); 
            div.className = 'ring';
            div.style.width = ringWidth(ring.size) + 'px'; 
            div.style.background = ring.color;
            div.textContent = ring.size;
            
            // insertBefore — вставляем div перед pole, так кольца окажутся визуально перед палкой
            // Так кольца окажутся визуально перед палкой
            el.insertBefore(div, el.querySelector('.pole'));
        }
    }
    
    // Счетчик шагов
    document.getElementById('counter').textContent = 
        'Step ' + (currentStep+1) + ' / ' + moves.length;
    
    // Блокируем кнопки если дошли до края
    document.getElementById('prevBtn').disabled = (currentStep < 0);
    document.getElementById('nextBtn').disabled = (currentStep >= moves.length - 1);
}

function nextStep() {
    if(currentStep >= moves.length - 1) return;
    currentStep++;
    var m = moves[currentStep];
    
    var ring = sticks[m.from].pop();   // Снимаем последний элемент
    sticks[m.to].push(ring);           // Кладем на другой
    
    // Информация о коде
    document.getElementById('info').textContent = 
        'Ring ' + m.size + ' stick ' + m.from + ' -> stick ' + m.to;
    
    render();
}

function prevStep() {
    if(currentStep < 0) return;
    var m = moves[currentStep];
    
    // Снимаем кольцо с to, возвращаем на from
    var ring = sticks[m.to].pop();
    sticks[m.from].push(ring);
    
    currentStep--;
    
    // Показываем информацию о предыдущем ходе (или начальное сообщение)
    if(currentStep >= 0) {
        var pm = moves[currentStep];
        document.getElementById('info').textContent = 
            'Ring ' + pm.size + ' stick ' + pm.from + ' -> stick ' + pm.to;
    } else {
        document.getElementById('info').textContent = 'Press Start or Next Step';
    }
    
    render();
}

function autoPlay() {
    if(autoInterval) { 
        clearInterval(autoInterval);  // Останавливаем таймер
        autoInterval = null; 
        document.getElementById('autoBtn').textContent = 'Auto Play'; 
        return; 
    }
    
    // Запускаем автоплей
    document.getElementById('autoBtn').textContent = 'Pause';
    
    // setInterval — вызывает функцию каждые 600мс
    autoInterval = setInterval(function(){
        // Если дошли до конца — останавливаемся
        if(currentStep >= moves.length - 1) {
            clearInterval(autoInterval); // Убиваем таймер
            autoInterval = null;
            document.getElementById('autoBtn').textContent = 'Auto Play';
            return;
        }
        nextStep();
    }, 600);
}

function resetAll() {
    // Если автоплей работает — останавливаем
    if(autoInterval) { clearInterval(autoInterval); autoInterval = null; }
    document.getElementById('autoBtn').textContent = 'Auto Play';
    
    sticks = [[],[],[]];
    currentStep = -1;
    
    // Заново сортируем кольца (большие первыми = внизу)
    initialRings.sort(function(a,b){ return b.size - a.size; });
    
    // Кладём все кольца обратно на начальный стержень
    for(var i = 0; i < initialRings.length; i++) 
        sticks[startStick].push(initialRings[i]);
    
    document.getElementById('info').textContent = 'Press Start or Next Step';
    render(); // Перерисовываем
}

render();
)JS";

    file << "</script></body></html>" << std::endl;
    file.close();

    // Открываем в браузере
    #ifdef _WIN32
        system("start hanoi.html");
    #elif __APPLE__
        system("open hanoi.html");
    #else
        system("xdg-open hanoi.html");
    #endif
}

#endif