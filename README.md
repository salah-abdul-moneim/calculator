# GTK4 Calculator (C)

**هيكلة المشروع**
- `main.c`: نقطة الدخول (تشغيل التطبيق) فقط.
- `src/ui.c`: بناء الواجهة (GTK) + الأزرار + منطق التفاعل.
- `src/calc_eval.c` + `include/calc_eval.h`: محرك حساب التعبيرات والدوال.
- `src/style_manager.c` + `include/style_manager.h`: تحميل الـCSS من الملفات ومتابعة وضع النظام (فاتح/داكن).
- `assets/dark.css` و`assets/light.css`: شكل البرنامج.

**التعديل السريع**
- لتغيير الألوان/الأحجام/الشكل: عدّل `assets/dark.css` و`assets/light.css`.
- لتغيير منطق الحساب أو إضافة دوال جديدة: عدّل `src/calc_eval.c`.
- لتغيير توزيع الأزرار أو شكل الواجهة: عدّل `src/ui.c`.

**البناء والتشغيل**
```bash
make
./calculator
```

