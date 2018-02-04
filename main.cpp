#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>

typedef double scalar;

sf::Color color_from_hsv(float h,float s,float v){
    float c = s*v;
    int h_bin = (int)(h*6);
    float x = c * (1 - fabs(h*6 - h_bin + (h_bin)% 2 -1));

    float r_p = 0, g_p = 0, b_p = 0;

    switch (h_bin){
        case 0:
            r_p = c;
            g_p = x;
            break;
        case 1:
            r_p = x;
            g_p = c;
            break;
        case 2:
            g_p = c;
            b_p = x;
            break;
        case 3:
            g_p = x;
            b_p = c;
            break;
        case 4:
            r_p = x;
            b_p = c;
            break;
        case 5:
        default:
            r_p = c;
            b_p = x;
            break;

    }

    float m = v-c;

    sf::Color col((r_p+m)*255, (g_p+m)*255, (b_p+m)*255);

    std::cout<<"("<<h<<", "<<s<<", "<<v<<") --> ("<<(r_p+m)*255<<", "<<(g_p+m)*255<<", "<<(b_p+m)*255<<")"<<std::endl;
    std::cout<<"OR ("<<h<<", "<<s<<", "<<v<<") --> ("<<col.r<<", "<<col.g<<", "<<col.b<<")"<<std::endl;
    return col;
}

class transform_t {
public:
    transform_t(scalar x1=0,scalar y1=0,scalar s=1): x(x1), y(y1), scale(s) {}
    scalar x;
    scalar y;
    scalar scale;
};

class succession {
public:
    succession(scalar x1=0,scalar y1=0,int iter1=0,bool div=false): x(x1), y(y1), iteration(iter1), diverges(div) {}
    scalar x;
    scalar y;
    int iteration;
    bool diverges;
};

class square{

public:
    square(bool filled_arg = false, bool agent_arg = false, bool goal_arg = false):
            filled(filled_arg), agent(agent_arg), goal(goal_arg){}

    bool filled;
    bool agent;
    bool goal; 

};

class grid{
public:
    grid(int width, int height, transform_t t): 
                        _width(width), _height(height)
    {
        _offset_x = t.x;
        _offset_y = t.y;
        _scale = t.scale;
        

        for(int i = 0; i<_width*_width;i++){
            square s;
            _squares.push_back(s);
        }
    }

    transform_t get_transform(){
        transform_t t(_offset_x, _offset_y, _scale);
        return t;
    }

    void transform(scalar off_x,scalar off_y,scalar s){
        _offset_x+=off_x/_scale;
        _offset_y+=off_y/_scale;
        _scale*=s;
    }

    void set_scale(scalar s){
        _scale = s;
    }

    void draw(sf::RenderWindow& window){
        sf::RectangleShape sq;
        sq.setSize(sf::Vector2f(_scale*_square_width, _scale*_square_width));
        sq.setOutlineThickness((1-_square_width)*0.5*_scale);
        sq.setOutlineColor(sf::Color::Black);
        
        for(int i = 0; i<_width; i++){
            for(int j = 0; j<_height; j++){
                if (_squares.at(i + j*_width).filled){
                    sq.setFillColor(sf::Color(200, 200, 200));
                } else {
                    sq.setFillColor(sf::Color(10, 10, 10));
                }

                sq.setPosition((i+_offset_x)*_scale,(j+_offset_y)*_scale);
                window.draw(sq);
            }
        }

        sf::CircleShape ag;
        ag.setRadius(_scale*_square_width*_agent_radius);
        ag.setOutlineThickness((1-_square_width)*_scale);
        ag.setOutlineColor(sf::Color::Black);

        for(int i = 0; i<_agents.size(); i++){
            int idx = _agents.at(i);
            sf::Vector2i pos = square_from_index(idx);
            ag.setFillColor(_colors.at(i));
            ag.setOrigin(_agent_radius*_square_width*_scale, _agent_radius*_square_width*_scale);
            ag.setPosition((pos.x+_offset_x+0.5*_square_width)*_scale,(pos.y+_offset_y+0.5*_square_width)*_scale);
            
            window.draw(ag);

        }

        sf::CircleShape goal;
        goal.setRadius(_scale*_agent_radius);
        goal.setOutlineThickness(_scale*0.1);
        goal.setFillColor(sf::Color::Transparent);
        
        for(int i = 0; i<_goals.size(); i++){
            int idx = _goals.at(i);
            if(idx>-1){


                sf::Vector2i pos = square_from_index(idx);
                goal.setOutlineColor(_colors.at(i));
                goal.setOrigin(_agent_radius*_scale, _agent_radius*_scale);
                goal.setPosition((pos.x+_offset_x+0.5*_square_width)*_scale,(pos.y+_offset_y+0.5*_square_width)*_scale);
                window.draw(goal);
            }
        }
    }

    void paint_square(sf::Vector2i pix){
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        if(idx > -1){
            _squares.at(idx).filled = true;
        }
    }

    void unpaint_square(sf::Vector2i pix){
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        if(idx > -1){
            _squares.at(idx).filled = false;
        

            if( _squares.at(idx).agent == true){
                remove_agent(idx);
            } else if(_squares.at(idx).goal == true){
                remove_goal(idx);
            }
        }
    }

    void add_agent(sf::Vector2i pix){
        _selected_agent = -1;
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        if(idx > -1 && _squares.at(idx).agent == false ){
            _squares.at(idx).filled = true;
            _squares.at(idx).agent = true;
            _agents.push_back(idx);
            _goals.push_back(-1);
            _colors.push_back(new_color());
        }
    }
    void select_agent(sf::Vector2i pix){
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        int ag_idx = -1;
        if(idx > -1 && _squares.at(idx).agent == true){
            for(int i = 0; i<_agents.size(); i++){
                if(_agents.at(i)==idx){
                    ag_idx = i;
                    break;
                }
            }
        }

        _selected_agent = ag_idx;
    }

    void add_goal(sf::Vector2i pix){
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        std::cout<<idx<<", "<<_selected_agent<<", "<<_squares.at(idx).goal<<std::endl;
        if(_selected_agent > -1 && idx > -1 && _squares.at(idx).goal == false){
            if (_goals.at(_selected_agent) > -1)
                _squares.at(_goals.at(_selected_agent)).goal= false;

            _squares.at(idx).filled = true;
            _squares.at(idx).goal= true;
            _goals.at(_selected_agent) = idx;
        }
    }

    void remove_goal(int idx){
        if(idx > -1 && _squares.at(idx).goal == true){

            _squares.at(idx).goal = false;

            for(int i = 0; i<_goals.size(); i++){
                if(_goals.at(i)==idx){
                    _goals.at(i) = -1;
                    break;
                }
            }
        }
    }

    void remove_goal(sf::Vector2i pix){

        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);

        remove_goal(idx);
        

    }

    void remove_agent(int idx){
        _selected_agent = -1;
        int ag_idx = -1;
        if(idx>0){
            if(_squares.at(idx).agent == true){
                for(int i = 0; i<_agents.size(); i++){
                    if(_agents.at(i)==idx){
                        ag_idx = i;
                        break;
                    }
                }
            }
        

            if (ag_idx > -1){                
                _squares.at(idx).agent = false;
    
                if(_goals.at(ag_idx)>-1)
                    _squares.at(_goals.at(ag_idx)).goal = false;
    
                _agents.erase(_agents.begin() + ag_idx);
                _goals.erase(_goals.begin() + ag_idx);
                _colors.erase(_colors.begin() + ag_idx);
            }
        }

    }

    void remove_agent(sf::Vector2i pix){       
        sf::Vector2f sq = pixels_to_squares(pix);
        int idx = get_square_index(sq);
        remove_agent(idx);        
    }

    void preprocess_instance(int* max_line,int* max_col,int* min_line,int* min_col){
        *(max_col) = _width-1;
        *(max_line) = _height-1;
        *(min_col) = 0;
        *(min_line) = 0;

        for(int i = 0; i<_width; i++){
            for(int j = 0; j<_height; j++){
                if(_squares.at(i+j*_width).filled){
                    *(max_col)  = *(max_col)  < i ? i : *(max_col);
                    *(max_line) = *(max_line) < j ? j : *(max_line);
                    *(min_col)  = *(min_col)  > i ? i : *(min_col);
                    *(min_line) = *(min_line) > j ? j : *(min_line);
                }
            }
        }
    }

    void save_to_file(){
        
        std::ofstream f;
        std::vector<int> idxs;
        int idx = 0;

        int max_line, max_col, min_line, min_col;

        preprocess_instance(&max_line, &max_col, &min_line, &min_col);

        f.open("untitled.cpf");

        f << "V =\n";

        for(int i = min_line; i<=max_line; i++){
            for(int j = min_col; j<=max_col; j++){
                if(_squares.at(i*_width+j).filled){
                    f << "(" << idx << ":-1)[";
                    int ag_idx = -1;
                    int goal_idx = -1;
                    for(int k = 0; k<_agents.size(); k++){
                        if (_agents.at(k) == i*_width+j){
                            ag_idx = k;
                            break;
                        }
                    }

                    for(int k = 0; k<_agents.size(); k++){
                        if (_goals.at(k) == i*_width+j){
                            goal_idx = k;
                            break;
                        }
                    }

                    f << ag_idx + 1 << ":";
                    f << goal_idx + 1 << ":" << goal_idx + 1 <<"]\n";
                    
                    idxs.push_back(i*_width+j);
                    idx++;
                }

            }
        }

        f << "E =\n";
        idx = 0;
        for(int i = min_line; i<max_line; i++){
            for(int j = min_col; j<max_col; j++){
                if(_squares.at(i*_width+j).filled){

                    int r_idx = -1;
                    int d_idx = -1;

                    //right_edge
                    if(_squares.at(i*_width+j+1).filled){
                        for(int k = 0; k<idxs.size(); k++){
                            if (idxs.at(k) == i*_width+j+1){
                                r_idx = k;
                                break;
                            }
                        }
                    }

                    //down_edge
                    if(_squares.at((i+1)*_width +j).filled){
                        for(int k = 0; k<idxs.size(); k++){
                            if (idxs.at(k) == (i+1)*_width +j){
                                d_idx = k;
                                break;
                            }
                        }
                    }
                    

                    if(d_idx > -1){
                        f<<"{"<<d_idx<<","<<idx<<"} (-1)\n";
                    }

                    if(r_idx > -1){
                        f<<"{"<<r_idx<<","<<idx<<"} (-1)\n";
                    }
                    
                    idx++;
                }
            }
        }


        f.close();
    }


    int get_square_index(sf::Vector2f sq){

        int x = (int)sq.x;
        int y = (int)sq.y;

        if (sq.x-x > _square_width || sq.y - y > _square_width ||
            x>=_width || y >=_width || x<0 || y <0)
            return -1;

        return x+y*_width;
    }
    sf::Vector2i square_from_index(int idx){
        sf::Vector2i res(idx % _width,(int)idx/_width);
        return res;
    }

    sf::Color new_color(){
        
        sf::Color ret =  color_from_hsv(_hue, 1, 0.7);

        _hue+= _hue_inc;
        if(_hue>=1){
            _hue_inc*=0.5;
            _hue = _hue_inc*0.5;
        }

        return ret;
    }

    sf::Vector2f pixels_to_squares(sf::Vector2i pix){
        sf::Vector2f sq;
        sq.x = pix.x/_scale - _offset_x;
        sq.y = pix.y/_scale - _offset_y;
        return sq;
    }

private:
    float _hue = 0;
    float _hue_inc = 2;
    int _width;
    int _height;
    int _selected_agent = -1;
    float _agent_radius = 0.3;
    scalar _scale;
    scalar _offset_y;
    scalar _offset_x;
    std::vector<square> _squares;
    std::vector<int> _agents;
    std::vector<int> _goals;
    std::vector<sf::Color> _colors;
    float _square_width = 0.95;



};

int main()
{
    int width = 1024;
    int height = 1024;
    bool stop = false;
   
    
    // create the window
    sf::RenderWindow window(sf::VideoMode(width, height), "CPF Maker");

    window.setActive(false);

    transform_t t(-32,-32,100);
    grid g(64, 64, t);    
    bool shift_before = false;

    sf::Vector2i center (width/2, height/2);
    sf::Vector2i current_mouse(500,500);
    sf::Vector2i prev_mouse = current_mouse;

    // run the main loop

    while (window.isOpen())
    {
        current_mouse = sf::Mouse::getPosition(window);
        sf::Vector2i delta = prev_mouse - current_mouse;

        // handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            if(event.type == sf::Event::Closed)
                window.close();

            if(event.type == sf::Event::MouseButtonPressed){
                if (event.mouseButton.button == sf::Mouse::Left){
                    if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
                        g.add_agent(current_mouse);
                    } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S)){
                        g.select_agent(current_mouse);
                    } else if(sf::Keyboard::isKeyPressed(sf::Keyboard::G)){
                        g.add_goal(current_mouse);
                    }
                }
            }

            if(event.type == sf::Event::KeyPressed){
                if(event.key.code == sf::Keyboard::U){
                    g.save_to_file();
                }
            }

            if (event.type == sf::Event::LostFocus)
                stop = true;

            if (event.type == sf::Event::GainedFocus)
                stop = false;
        }

        if(!stop){

        

            if(sf::Mouse::isButtonPressed(sf::Mouse::Right)){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)){

                    float scale_mult =  sqrt((current_mouse.x - center.y)*(current_mouse.x - center.x) + (current_mouse.y - center.y)* (current_mouse.y - center.y));
                    scale_mult /=  sqrt((prev_mouse.x - center.y)*(prev_mouse.x - center.x) + (prev_mouse.y - center.y)* (prev_mouse.y - center.y));
                    if(scale_mult<5 && scale_mult> 0.2){
                        g.transform(-1024*(scale_mult-1)*0.5/scale_mult,-1024*(scale_mult-1)*0.5/scale_mult,scale_mult);
                    }
                }
                else {
                    g.transform(-delta.x, -delta.y, 1);         
                }
            }

            if(sf::Mouse::isButtonPressed(sf::Mouse::Left)){
                if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)){
                    g.unpaint_square(current_mouse);
                } else if(!sf::Keyboard::isKeyPressed(sf::Keyboard::A)){
                    g.paint_square(current_mouse);
                }
            }
        }
        prev_mouse = current_mouse;

        window.clear();
        g.draw(window);
        window.display();

        //sprite.setTexture(texture);        
    }

    return 0;
}