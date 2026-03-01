Beskrivelse:

Dette er et simpelt OpenGL prosjekt som lar deg kjøre compute-shadere skrevet i GLSL,
nedenfor er korte forklaringer på hva hver .cpp fil og deres funksjoner gjør,
og litt om hva de forskjellige shader-typene er og hvordan de brukes i prosjektet

Krav:
- OpenGL 4.3+ (compute shaders krever dette)
- cmake 3.10+
- en moderne C++-kompilator (C++11 eller nyere)
- Compute-shadere forventes å bruke layout(rgba8, binding = 0) for output
- Prosjektet antar vanligvis local_size_x/y = 16

Filoversikt:
- `src/main.cpp`  
  Hovedinn- og utgangspunkt:
  - Initialiserer GLFW og GL loader, oppretter GL-kontekst og hovedløkke.
  - Oppretter og endrer størrelse på render-teksturen med `createTextureRGBA8` / `resizeTextureRGBA8`.
  - Laster compute- og blit-program med `buildComputeProgramFromFile` og `buildProgramFromFiles`.
  - Kjører hovedløkken: setter uniforms (`uTime`, `dt`), binder SSBO/tekstur, dispatche compute, barrier, og blit til skjerm.
  - dette programmet trengs ikke endres på for å kjøre nye shaders.

- `src/gl_texture.cpp`  
  Tekstur-hjelpere:
  - `createTextureRGBA8(int w, int h)` — oppretter en `GL_RGBA8` 2D-tekstur med fornuftige parametre.
  - `resizeTextureRGBA8(GLuint tex, int w, int h)` — reallokerer teksturlagring ved vindusstørrelsesendring.
  Hvordan bruke: kall `createTextureRGBA8` med ønsket størrelse.
	- For compute-skriving: bind med `glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8)`
	-  For sampling i fragment shader: bind til en teksturenhet og sett sampler-uniform.

- `src/file_io.cpp`  
  Enkel fil-leser:
  - `readTextFile(const char* path)` — leser hele filen til en `std::string`, kaster `std::runtime_error` ved feil.
  Hvordan bruke: bruk for å hente GLSL-kilde før kompilering, f.eks. i shader-byggere.

- `src/gl_shader.cpp`  
  Shader-kompilering og linking:
  - `compileShader(GLenum type, const char* source, const char* label)` — kompilerer shader og skriver logg ved feil.
  - `linkProgram(const std::vector<GLuint>& shaders, const char* label)` — linker shaders til et program.
  - `buildProgramFromFiles(const char* vertPath, const char* fragPath)` — leser filer og returnerer linket program.
  - `buildComputeProgramFromFile(const char* compPath)` — leser og bygger compute-program.
  Hvordan det brukes av main: kaller `buildComputeProgramFromFile("shaders/foo.comp")` for compute. 
	- For blit brukes `buildProgramFromFiles("shaders/fullscreen.vert", "shaders/blit.frag")`. 
	- Funksjonene kaster ved feil og skriver kompilerings-/link-logg til stderr.

- `src/menu.cpp`  


- `src/pingpong.cpp`  


hvordan bruke shaders:
- Compute-shadere (`.comp`)  
  - Beregnet for å skrive direkte til en output-tekstur bundet som et bilde via binding 0 (f.eks. `layout(rgba8, binding = 0) uniform writeonly image2D outImage;`).
  - Vanlige uniforms: `uTime` (float), `dt` (float). Threadgroup-størrelse i koden er vanligvis delt i 16×16 (dispatche beregnet fra teksturstørrelse).
  - Navnekonvensjon (prosjektet): render-/update-par kan navngis systematisk i shaders-mappen (inspill fra Frederik?).
  Hvordan bruke: skriv en compute som tar `outImage` og eventuelle uniforms,
	- kall `glDispatchCompute` med grid beregnet fra output-størrelsen og sørg for `glMemoryBarrier` før tekstursampling.
	- for å legge til en ny compute-shader:
  1. Legg til en .comp-fil i shaders/.
  2. Sørg for layout(rgba8, binding=0) output.
  3. (Valgfritt) bruk uTime og dt.
	- For simple shadere trenger man kun lage en compute-shader (er enda usikker på når vi må lage nye vertex/fragment shadere).

- Vertex-shadere (`.vert`)  
  - Brukes her for et enkelt fullscreen-blit. Vertex-shaderen trenger ingen input-attributter; den genererer et fullskjerms-trekantoppsett for fragment-trinnet.
  Hvordan bruke: bygg sammen med fragment-shader for blit/presentasjon.

- Fragment-shadere (`.frag`)  
  - Fragment-shaderen for blit leser fra en `sampler2D` (vanligvis uniform `uTex`) og skriver farge til skjermen.
  Hvordan bruke: bind output-teksturen til en teksturenhet (`GL_TEXTURE0`) og sett `uTex` til 0 før tegning.

Kort om feilsøking
- Shader-kompilering/link-feil logges til stderr med innhold fra GL-info-loggene.
- `readTextFile` kaster hvis shaderfilen ikke finnes — pass på at relative stier (`shaders/`) er korrekte.

Litt intuisjon om shader-typer:
 - Vertex shadere prosseserer hver vertex (hjørne eller punkt i et mesh) og bestemmer hvor det skal tegnes på skjermen.
	- Vertex shaderen gir en relativ posisjonsvektor for hvert vertex, og kan også sende data videre til fragment shaderen,
	- dette kalles ofte uv-koordinater.

 - Fragment shadere prosserer hvert fragment av et bilde (som vanligvis tilsvarer piksler) og bestemmer fargen på det fragmentet

 - Compute shadere er veldig generelle og lever utenfor grafikk-pipelinen.
	- de brukes i praksis til å gjøre alle slags beregninger på GPUen, og sender så resultatet til en tekstur eller buffer som kan brukes senere i grafikk-pipelinen,
	- resultatet leses ofte av fragment shadere (og noen ganger vertex shadere) for å bestemme hvordan ting skal tegnes på skjermen.
	- et er også vanlig å bruke flere compute shadere i en serie for å gjøre mer komplekse beregninger,
	- der hver shader tar resultatet fra den forrige som input, denne teknikken kalles ofte "ping-pong",
	- og kan også brukes for å lagre data om mellom frames av simulasjoner eller animasjoner.

