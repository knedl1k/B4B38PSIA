#import "@preview/cetz:0.2.2": canvas, plot

#set par(justify: true)
#set text(
  font: "New Computer Modern",
  lang:"cs",
  size: 10.5pt,
)

#set page(
  paper: "a4",
  header:[
    
  ],
  
  footer:[
    #set align(left)
    Praha, léto 2024
    #set align(center)
    #counter(page).display(
      "1",
    )
  ],
  
)

#set align(center)
= Nevýhody metody _Stop-and-Wait_ při \ rostoucím zpoždění mezi koncovými uzly
Jakub Adamec --  adamej14\@fel.cvut.cz \
Daniel Petránek -- petrada6\@fel.cvut.cz
\
\
\
#set align(left)
= Popis
Hlavní nevýhodou algoritmu _Stop-and-Wait_ je nízká datová propustnost. Při rostoucím zpoždění mezi koncovými uzly se snižuje efektivita komunikace, protože odesílatel musí čekat na potvrzení každého vyslaného paketu, před tím, než může odeslat další paket, což ve výsledku zpomaluje přenos dat. Komunikace je tedy při rostoucím zpoždění velmi neefektivní.

Definujeme datovou propustnost $P$ jako počet dat $D$, který se přenese za dobu $T$:
$ P = D/T $

Při metodě _Stop-and-Wait_ musí odesílatel pro odeslání jednoho paketu čekat dobu dvou zpoždění mezi koncovými uzly $τ$ (za předpokladu bezchybné komunikace). Při zvyšování zpoždění tedy propustnost nepřímo úměrně klesá:
$ P ∝ 1 / (2τ) $

K tomu, abychom mohli vyjádřit propustnost přesněji, uvažujme velikost paketu $L$ a zpoždění $τ$. Čas potřebný k přenosu jednoho paketu a obdržení potvrzení je:
$ T = 2τ + L/R $

kde $R$ je přenosová rychlost linky. Datová propustnost je pak:
$ P = L / (2τ + L/R) $

= Měření
Výše odvozený vztah jsme experimentálně ověřili pomocí měření za pomoci programu NetDerper. Pomocí metody _Stop-and-Wait_ jsme posílali soubor o velikosti 230 KB a naměřili následující hodnoty pro dobu přenosu souboru:

#set align(center)
#table(
columns: 6,
[Zpoždění paketů / ms], [1], [10], [50], [100], [200],
[Doba přenosu souboru / s], [1.99], [4.46], [14.75], [26.39], [49.53],

)
#set align(left)

= Závěr
Naměřená data odpovídají popsanému matematickému předpokladu. Jak lze vidět z výsledků, doba přenosu se značně zvyšuje se zvyšujícím se zpožděním mezi koncovými uzly, což potvrzuje, že propustnost při metodě _Stop-and-Wait_ nepřímo úměrně klesá se zvyšujícím se zpožděním.
