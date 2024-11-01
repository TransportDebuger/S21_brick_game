void printing() {
  addstr("This was printed using addstr\n");
  refresh();

  addstr("The following letter was printed using addch:-\n");
  addch('a');
  refresh();

  printw("\nThese numbers were printed using printw %d %f\n", 123, 456.789);
  refresh();
}

void moving_and_sleeping() {
  int row = 5, col = 0;
  curs_set(0);

  for (char c = 65; c <= 90; c++) {
    move(row++, col++);
    addch(c);
    refresh();
    napms(100);
  }

  row = 5;
  col = 3;

  for (char c = 97; c <= 122; c++) {
    mvaddch(row++, col++, c);
    refresh();
    napms(100);
  }

  curs_set(1);

  addch('\n');
}

void colouring() {
  if (has_colors()) {
    if (start_color() == OK) {
      init_pair(1, COLOR_YELLOW, COLOR_RED);
      init_pair(2, COLOR_GREEN, COLOR_GREEN);
      init_pair(3, COLOR_MAGENTA, COLOR_CYAN);

      attrset(COLOR_PAIR(1));
      addstr("Yellow and red\n\n");
      refresh();
      attroff(COLOR_PAIR(1));

      attrset(COLOR_PAIR(2) | A_BOLD);
      addstr("Green and green A_BOLD\n\n");
      refresh();
      attroff(COLOR_PAIR(2));
      attroff(A_BOLD);

      attrset(COLOR_PAIR(3));
      addstr("Magenta and cyan\n");
      refresh();
      attroff(COLOR_PAIR(3));
    } else {
      addstr("Cannot start colours\n");
      refresh();
    }
  } else {
    addstr("Not colour capable\n");
    refresh();
  }
}