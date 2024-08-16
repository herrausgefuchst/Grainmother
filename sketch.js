// Transmit-Buffers to BELA
let TXSlider = []
let TXButton = [1, 1, 1, 1, 1, 1, 1, 1, 1];
let TXGuiControls = [];

// variables for positioning the elements
let column = [20, 160, 310, 367];
let row = [];
let numRows = 50;
let bgimage;

// display
let displayLines = [];
let num_displaylines = 10;

// arrays of elements
let header = [];
let slider = [];
let choice = [];
let toggle = [];
let button = [];


class Slider
{
    constructor (idx, pos, name, unit, min, max, start, step, log, guiOnly)
    {
        this.idx = idx;
        this.pos = pos;
        this.min = min;
        this.max = max;
        this.range = max - min;
        this.log = log;
        this.name = createSpan(name);
        this.unit = createSpan(unit);
        this.guiOnly = guiOnly;

        // linear Fader
        if (this.log === false)
        {
            this.slider = createSlider(min, max, start, step);
            this.var = createSpan(this.slider.value());
        }
        // logarithmic Fader TODO: change Lin 2 Log
        else
        {
            this.start = (start - this.min) / (this.range - this.min)
            this.slider = createSlider(0, 1, this.start, 0);
            this.var = createSpan(round(this.lin2log()),3);
        }

        // styling fader
        this.name.style('font-family', 'Courier');
        this.name.style('font-size', '13px');
        this.unit.style('font-family', 'Courier');
        this.unit.style('font-size', '13px');
        this.var.style('font-family', 'Courier');
        this.var.style('font-size', '13px');

        // position faders
        this.name.position(column[0], row[this.pos]);
        this.slider.position(column[1], row[this.pos]);
        this.var.position(column[2], row[this.pos]);
        this.unit.position(column[3], row[this.pos]);

        // change happens
        this.slider.input(this.reportChange.bind(this));

        // add this as element in Buffer to send over to BELA
        if (this.guiOnly === false) { TXSlider.push(this.slider.value()/100); }
        else { TXGuiControls.push(this.slider.value()); }
    }

    reportChange()
    {
        // linear fader
        if (this.log === false)
        {
            // change GUI
            this.var.html(round(this.slider.value(), 3));
            // add new value to buffer
            if (this.guiOnly === false) { TXSlider[this.idx] = this.slider.value()/100; }
            else { TXGuiControls[this.idx] = this.slider.value(); }
        }
        
        // logarithmic fader
        else
        {
            // change GUI
            this.var.html(round(this.lin2log(), 3));
            // add new value to buffer
            if (this.guiOnly === false) { TXSlider[this.idx] = this.lin2log(); }
            else { TXGuiControls[this.idx] = this.lin2log(); }
        }
        print(this.name.html() + ': ' +  this.var.html() + ' ' + this.unit.html());
    }

    lin2log()
    {
        return this.range * (pow(2, this.slider.value()) - 1) + this.min;
    }
}

class Choice
{
    constructor(idx, pos, name, choices)
    {
        this.idx = idx;
        this.pos = pos;
        this.name = createSpan(name);
        this.choice = createSelect();
        
        // add choices to the selector menue
        for (let i = 0; i < choices.length; i++) this.choice.option(choices[i], i);
        
        // styling element
        this.choice.style('background-color', '#607D8B7F');
        this.choice.style('border-radius', '3px');
        this.choice.style('width', '250px');
        this.choice.style('font-family', 'Courier');
        this.name.style('font-family', 'Courier');
        this.name.style('font-size', '13px');

        // positioning element
        this.name.position(column[0], row[this.pos]);
        this.choice.position(column[1], row[this.pos]);

        // report change
        this.choice.changed(this.reportChange.bind(this));

        // add this as element in Buffer to send over to BELA
        TXGuiControls.push(this.choice.value());
    }

    reportChange()
    {
        TXGuiControls[this.idx] = this.choice.value();
        print(this.name.html() + ': ' +  this.choice.value());
    }
}

class Button
{
    constructor(idx, pos, name)
    {
        this.idx = idx;
        this.pos = pos;
        this.name = createSpan(name);
        this.button = createButton(name);
        this.buttonState = 1; // momentary between 1 and 0 to report a click

        // positioning element
        this.name.position(column[0], row[this.pos]);
        this.button.position(column[1], row[this.pos]-3);

        // styling element
        this.name.style('font-family', 'Courier');
        this.name.style('font-size', '13px');
        this.button.style('font-family', 'Courier')
        this.button.style('border-radius', '0%');
        this.button.style('background-color','rgb(238,238,238)');
        this.button.style('width','250px');
        this.button.style('color', '#0F1619F2');

        // mouse over style
        this.button.mouseOver(()=>this.button.style('background-color: #DADADA;'));
        this.button.mouseOut(()=>this.button.style('background-color: rgb(238,238,238);'));
        
        // report change
        this.button.mousePressed(this.reportChange.bind(this, 0));
        this.button.mouseReleased(this.reportChange.bind(this, 1));

        // add this as element in Buffer to send over to BELA
        TXButton[this.idx] = this.buttonState;
    }

    reportChange(state)
    {
        this.buttonState = state;
        // add new value to Send Buffer
        TXButton[this.idx] = this.buttonState;
        print(this.name.html() + ': ' + this.buttonState);
    }
}

class Header
{
    constructor(pos, text)
    {
        this.pos = pos;
        this.header = createSpan(text);
        this.header.position(column[0], row[pos]);
        this.header.style('font-family', 'Courier');
    }
}

function setup()
{
    createCanvas(windowWidth, windowHeight);
    bgimage = loadImage("https://i.ibb.co/3p8Yq5y/IMG-0292.jpg");
    background('rgba(200,200,100,0.9)');
  
    let title = createSpan("MULTIEFFECT - Version 0.01 <br> 10.10.2023");
    title.position(480,20);
    title.style('font-family', 'Courier');
    title.style('font-weight', 'bold');
    
    for (let i = 0; i < numRows; i++) row.push(20 + i * 27);
    
    let idxS = 0;
    let idxB = 0;
    let idxGUI = 0;
    let pos = 0;
    
    for (i = 0; i < 10; i++)
    {
      displayLines[i] = createSpan("");
      displayLines[i].position(485,115+i*16);
      displayLines[i].style('font-family', 'Courier');
      displayLines[i].style('color', '#FFFFFF');
      displayLines[i].style('font-size', '17px');
    }
  
    header.push(new Header(pos, 'Potentiometers'));
    slider.push(new Slider(idxS, ++pos, 'Pot. 1', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 2', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 3', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 4', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 5', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 6', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 7', '%', 0, 100, 0, 0, false, false));
    slider.push(new Slider(++idxS, ++pos, 'Pot. 8', '%', 0, 100, 0, 0, false, false));

    header.push(new Header(++pos, 'Effects'));
    button.push(new Button(idxB, ++pos, 'FX 1'));
    button.push(new Button(++idxB, ++pos, 'FX 2'));
    button.push(new Button(++idxB, ++pos, 'FX 3'));
    header.push(new Header(++pos, 'Action'));
    button.push(new Button(++idxB, ++pos, 'ACTION'));
    header.push(new Header(++pos, 'Upper Row'));
    button.push(new Button(++idxB, ++pos, 'Bypass'));
    button.push(new Button(++idxB, ++pos, 'Tempo'));
    header.push(new Header(++pos, 'Menu'));
    button.push(new Button(++idxB, ++pos, 'Up'));
    button.push(new Button(++idxB, ++pos, 'Down'));
    button.push(new Button(++idxB, ++pos, 'Menu / Exit'));
    button.push(new Button(++idxB, ++pos, 'Save / Enter'));
  
    header.push(new Header(++pos, 'Input Controls'));
    choice.push(new Choice(idxGUI, ++pos, 'Input Type', ['File', 'Sine Wave', 'Audio In']));
    choice.push(new Choice(++idxGUI, ++pos, 'Choose Track', ['Waves', 'Drums', 'Vocals', 'Orchestra', 'Synth']));
    slider.push(new Slider(++idxGUI, ++pos, 'Sine Frequency', 'Hz', 50, 800, 150, 0, true, true));
    slider.push(new Slider(++idxGUI, ++pos, 'Input Volume', '%', 0, 100, 50, 0, false, true));
}

function draw()
{
    // TRANSMIT
    Bela.data.sendBuffer(0, 'float', TXSlider);
    Bela.data.sendBuffer(1, 'float', TXButton);
    Bela.data.sendBuffer(2, 'float', TXGuiControls);
  
    // draw backgrounds
    background(bgimage);
    
    noStroke();
    fill('rgba(255,255,255,0.3)');
    let ui_background = rect(8,10,420,900,10);
    
    strokeWeight(16);
    fill('black');
    rect(470, 110, 540, 190, 10);
  
    // RECEIVE Display
    let RX_Display = [];
    let lines = [];
    for (let n = 0; n < num_displaylines; n++)
    {
    	RX_Display[n] = Bela.data.buffers[n+4];
    	lines[n] = '';
        for (let i = 0; i < RX_Display[n].length; i++) lines[n] += RX_Display[n][i];
        displayLines[n].html(lines[n]);
    }
  
    // RECEIVE Leds
    let RX_LEDs = Bela.data.buffers[3];

    rect(122, 280, 23, 90, 10);
    rect(122, 391, 23, 30, 10);
    rect(122, 443, 23, 60, 10);
    
    noStroke();
    let ledcolumn = 134;
    let ledrow = 298;

    let r = 255;
    let g = 60;
    let b = 0; 
    for (let n = 0; n < 3; n++)
    {
        fill('rgba('+r+','+g+','+b+','+ RX_LEDs[n] + ')')
        ellipse(ledcolumn, ledrow+n*27, 15);
    } 
  
    r = 255;
    g = 60;
    b = 0; 
    fill('rgba('+r+','+g+','+b+','+ RX_LEDs[4] + ')')
    ellipse(ledcolumn, ledrow+4*27, 15);
  
    ledrow += 6*27;
    r = 255;
    g = 60;
    b = 0; 
    for (let n = 0; n < 2; n++)
    {
        fill('rgba('+r+','+g+','+b+','+ RX_LEDs[n+4] + ')')
        ellipse(ledcolumn, ledrow+n*27, 15);
    } 

}
