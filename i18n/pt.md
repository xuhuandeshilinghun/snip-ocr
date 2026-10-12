**Readme Language / Idioma：**
[English](../README.md)｜[日本語](ja.md)｜[한국어](ko.md)｜[Русский](ru.md)｜[Français](fr.md)｜[Deutsch](de.md)｜[Español](es.md)｜[Português](pt.md)｜[简体中文](zh.md)｜[繁體中文](tw.md)


💖 Apoie este projeto: [Ko‑fi](https://ko-fi.com/edenls)


# SnapOCR

 
> Utilitário de captura de tela, adesivos de área de trabalho e OCR offline de alto desempenho, desenvolvido em C++17 / Qt6 para a plataforma Windows.

SnapOCR é uma ferramenta de área de trabalho tudo‑em‑um de código aberto para Windows 10 / 11 x64. Integra captura de tela de alto desempenho, visualização flutuante de adesivos de área de trabalho, anotação de imagens, reconhecimento OCR multilíngue offline, captura retardada em loop e sistema de atualização automática.
**Idiomas suportados pela interface: chinês simplificado, chinês tradicional, inglês, japonês, coreano, alemão, francês, russo, espanhol e português; o reconhecimento OCR funciona para todos esses idiomas.**

Todo o processamento de imagem e reconhecimento de texto são executados **localmente no seu dispositivo. As capturas de tela nunca são enviadas para a nuvem, protegendo a sua privacidade.**

Ideal para extração de conteúdos de escritório, auxílio à leitura e tradução, extração de texto de documentos web, captura de materiais para programadores, anotação de capturas para demonstrações de ensino e muitos outros cenários.

Desenvolvido com APIs nativas do Windows e Qt6. Suporta múltiplos monitores com alto‑DPI, oferece temas claro e escuro, interface totalmente internacionalizada e executa em segundo plano na bandeja do sistema.

---

## Funcionalidades

### 📷 Módulo de captura de tela

- **Captura por tecla de atalho global**: Implementado por meio do hook de teclado de baixo nível do Windows `WH_KEYBOARD_LL`. Acione a captura com uma tecla independente da janela em foco.
- **Seleção de região livre**: Arraste o mouse para definir a área, com oito pontos de redimensionamento. Ajuste fino com teclas de seta: 1 px por passo, `Shift + tecla de seta` move 10 px para seleção precisa de pixels.
- **Ajuste automático às bordas das janelas**: Inspirado no Snipaste. A borda da seleção se ajusta automaticamente às janelas com limite padrão de 8 px para capturar janelas inteiras rapidamente.
- **Detecção precisa de janelas e controles**: Usa `WindowFromPoint` + `RealChildWindowFromPoint` para identificar botões, caixas de entrada, barras de ferramentas e selecionar diretamente a área desses componentes.
- **Obtenção dos limites reais da janela via DWM**: Chama `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` para ignorar sombras e obter as dimensões reais renderizadas da janela.
- **Lupa de pixels**: Sobreposição na captura, exibe valores de cor RGB / HEX e coordenadas da tela. Alterna formatos de cor para captura de cor e inspeção de pixels.
- **Rótulo de dimensão da seleção**: Rótulo `W × H` com fundo azul arredondado. A cor do texto se adapta automaticamente aos temas claro e escuro.
- **Barra de ferramentas da captura**: Aparece na parte inferior após a seleção. Suporta fixar adesivo, reconhecimento OCR, leitura de código QR, copiar imagem, salvar como PNG e cancelar.

### 🖼️ Adesivos de área de trabalho e anotações

- **Adesivos flutuantes**: Cole a captura diretamente na tela na posição original, semelhante ao Snipaste. Suporta totalmente DPR para evitar imagens borradas em telas de alta resolução.
- **Conjunto completo de ferramentas de anotação**: Clique com o botão direito para abrir a barra de desenho: seleção, caneta, seta, retângulo, elipse, destaque, mosaico de desfoque, nota de texto, borracha.
- **Transformação de imagem**: Os adesivos suportam escala de 20%‑500%; rotação de 90° horária / anti‑horária, espelhamento horizontal e vertical. Todas as operações usam a resolução física original sem perda de qualidade.
- **Exportação com anotações incorporadas**: Mescla todas as anotações na imagem original e exporta um arquivo PNG mantendo a resolução original.
- **Painel de captura de cor da tela**: Ferramenta independente para amostrar qualquer pixel da tela, compatível com interface multilíngue.
- **Alternância de sombra da janela do adesivo**: Ative ou desative o efeito de sombra para combinar com o visual de cantos arredondados do Windows 11.
- **Gerenciamento de grupos de adesivos**: Vários grupos (grupo padrão + grupos personalizados). Troque de grupo para ocultar os adesivos do grupo atual e restaurar os de outro grupo.
- **Histórico persistente de capturas**: As capturas originais são salvas como PNG na pasta `history`. Posição, escala e estado das anotações são guardados em arquivos `.sp1` com ofuscação leve XOR+Base64 (não é criptografia forte, apenas evita modificações acidentais).
- **Restauração automática ao reiniciar**: Ao abrir o programa novamente, todos os adesivos recuperam suas posições, escalas e grupos anteriores.

### 🔍 Reconhecimento de texto OCR offline

- **Carregamento paralelo de múltiplos modelos**: Carrega vários modelos ao mesmo tempo. Suporta texto misto chinês‑inglês, coreano, cirílico russo e alfabetos latinos.
- **Detecção automática de idioma**: O algoritmo analisa a proporção de caracteres na imagem e troca automaticamente o modelo OCR correspondente, sem necessidade de escolha manual.
- **Inferência CPU com ONNX Runtime**: Executa‑se totalmente na CPU. **Não requer placa de vídeo dedicada**, funciona em notebooks comuns.
- **Thread de fundo sem bloqueio**: As tarefas OCR rodam em uma thread QThread independente para que a interface não trave durante o reconhecimento.
- **Janela com resultado do reconhecimento**: Texto editável, suporte a cópia com um clique para a área de transferência e salvamento em arquivo local.

### ⏱️ Captura retardada

- **Captura retardada e em loop**: Defina atraso de 1‑10 segundos e número de capturas repetidas para salvar automaticamente em lote.
- **Notificação toast do sistema**: Aviso nativo do Windows quando a sequência terminar, mostra caminho de salvamento e quantidade de capturas. Clique na notificação para copiar o caminho do arquivo.

### 🔄 Sistema de atualização automática

- **Verificação de versão**: Verifica novas versões na inicialização; também é possível verificar manualmente nas configurações.
- **Mecanismo de atualização delta**: Gera pacotes completos e pacotes de patch baseados na versão de referência. Apenas arquivos alterados são baixados para reduzir o tamanho do download.
- **Roteamento de duas fontes**: Usuários da China continental usam o espelho Gitee; usuários do exterior usam GitHub Releases para resolver diferenças de acesso à rede.
- **Atualizador independente**: `updater.exe` construído com Win32 API nativo + minizip, sem dependências do Qt.
- **Preservar configurações e dados do usuário**: Faz backup do arquivo `.config_settings.json` durante a atualização. Restaura preferências depois; teclas de atalho, temas e grupos de adesivos são mantidos.
- **Fluxo de reinício após atualização**: Ao terminar a atualização, chama `ShellExecuteExW` para executar a nova versão com o parâmetro `--restart`, ignorando o mutex de instância única para reiniciar corretamente.

### ⚙️ Integração com o sistema

- **Execução na bandeja do sistema**: Minimiza para a bandeja. Menu de botão direito: captura de tela, OCR, captura retardada, grupos de adesivos, limpar histórico, preferências, reiniciar e sair.
- **Internacionalização da aplicação**: Interface nativa em chinês simplificado `zh_CN` e inglês `en_US`. Troque de idioma em tempo de execução; caixas de diálogo do Qt também são traduzidas.
- **Adaptação de tema claro‑escuro**: Segue o sistema ou troque manualmente. Máscaras de sobreposição, separadores de menu e sombras de janela se adaptam.
- **Mutex de instância única**: Mutex global impede múltiplas execuções do programa. Um parâmetro especial ignora essa verificação durante o reinício após atualização.
- **Compatibilidade com caminhos UTF‑8 com caracteres chineses**: Contorna um bug nativo da API Qt que pode retornar valores vazios para caminhos do Windows com caracteres chineses.

---

## Pré‑visualização

| Região da captura | Anotação com adesivo | Resultado OCR |
| --- | --- | --- |
| ![captura](../docs/images/capture.png) | ![adesivo](../docs/images/sticker.png) | ![ocr](../docs/images/ocr.png) |

## Download e instalação

1. Baixe a versão mais recente em [Releases](https://github.com/xuhuandeshilinghun/snip%E2%80%91ocr/releases): `SnapOCR‑<version>‑win64.zip`
2. Extraia o arquivo compactado para qualquer pasta local. Software portátil, não precisa de instalador.
3. Baixe o pacote separado dos modelos OCR e extraia para a subpasta `model/` dentro da pasta do programa.
4. Execute `SnapOCR.exe`.

> 
> **Requisito obrigatório do sistema: Windows 10 / Windows 11 x64**
> Os arquivos dos modelos OCR são grandes e não estão inclusos no pacote principal.
> Evite extrair em pastas protegidas pelo sistema como `C:\Windows` ou `Program Files` para evitar problemas de permissão de leitura e gravação.

## Atalhos de teclado

| Atalho | Função |
| --- | --- |
| `F1` | Iniciar modo de captura de tela |
| `Ctrl+Shift+S` | Capturar tela e executar reconhecimento OCR |
| `Esc` | Sair / cancelar captura |
| `Roda do mouse` | Ajustar a seleção em 1 pixel |
| `C` | Copiar valor de cor da lupa |

## Pilha tecnológica

- Linguagem: C++17
- Interface gráfica: Qt 6.5.3+
- Sistema de compilação: CMake 3.20+ / MSVC
- Inferência OCR: ONNX Runtime 1.18.1 CPU
- Processamento de imagem: OpenCV 4.x
- Análise JSON: nlohmann/json
- Compactação e descompactação Zip: minizip
- Interfaces do Windows: Win32 API, DWM, hook de teclado de baixo nível
- Compilador: MSVC2019 / MSVC2022

## Avisos de privacidade e segurança

1. Todo processamento de capturas, renderização de adesivos e reconhecimento OCR acontece localmente. O software **nunca envia automaticamente capturas ou textos reconhecidos para a internet.**
2. A função de atualização automática acessa servidores GitHub / Gitee apenas para validar versões e baixar pacotes de atualização.
3. As capturas originais são salvas como PNG na pasta `history`. Limpe manualmente essa pasta se capturar senhas, documentos de identidade ou materiais sensíveis.
4. Os arquivos `.sp1` e `index.dat` usam ofuscação XOR+Base64 simples **que NÃO é criptografia segura. Não armazene dados altamente confidenciais nesses arquivos.**
5. Você pode desligar o salvamento do histórico de capturas nas configurações do programa.

## Limitações conhecidas

- Atualmente só suporta Windows 10 / Windows 11 x64. Linux e macOS ainda não são suportados.
- O OCR usa a CPU; a velocidade depende do desempenho do processador.
- Os arquivos dos modelos OCR são grandes e precisam ser baixados e instalados manualmente.
- Configurações com múltiplos monitores e DPI misto devem ser testadas no uso real; podem ocorrer problemas de compatibilidade em configurações de hardware raras.

## Contribuindo com o projeto

Relatos de bugs e sugestões de funcionalidades por meio de Issues e Pull Requests são bem‑vindos.

1. Faça um fork deste repositório
2. Crie uma branch de funcionalidade: `git checkout -b feature/your‑feature‑name`
3. Confirme suas alterações
4. Envie a branch e crie uma PR

 
> Observação para envio: Não envie artefatos de compilação, conteúdo da pasta build ou arquivos grandes de modelos. Use submódulos git para bibliotecas de terceiros.

## Desenvolvedor: compilação, empacotamento e publicação

 
> Não destinado a usuários finais, apenas para mantenedores do projeto.

### Pré‑processamento de recursos antes da publicação

Antes de cada empacotamento de release, execute obrigatoriamente os scripts de compilação de recursos:

```
# Compilar recursos de localização multilíngue
python tools\compile_locales.py
# Empacotar recursos de imagem
python tools\pack_img_res.py
```

### Lógica de escolha do pacote de atualização (regras do atualizador automático)

| Versão instalada localmente | Tipo de pacote para download |
| --- | --- |
| V1.0.1 (versão oficial anterior) | Pacote delta (menor tamanho, poucos MB) |
| V1.0.0 (versão de referência) | Pacote completo (diferenças calculadas em relação à base 1.0.0) |
| Outras versões antigas | Pacote completo (diferenças calculadas em relação à base 1.0.0) |
| Não instalado | **É necessário instalar primeiro a versão de referência V1.0.0** |


> ⚠️ `build_release_1.0.0` é o diretório de referência global. **Gere‑o apenas uma vez, mantenha‑o permanentemente, não apague nem modifique**. Todas as atualizações delta posteriores dependem dessa base.

### Exemplos de script make_release.py

```
# 1.【Executar apenas uma vez】Criar versão de referência 1.0.0
python tools/make_release.py --version 1.0.0 --no-build

# 2.Publicar 1.0.1 (base fixa em 1.0.0, compara com build_release_1.0.0 antigo)
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --base-dir build_release_1.0.0 --no-build

# 3.Publicar 1.0.2 (base fixa em 1.0.0, compara com build_release_1.0.1 antigo)
python tools/make_release.py --version 1.0.2 --make-update --old build_release_1.0.1 --base-dir build_release_1.0.0 --no-build

# 4.Publicar 1.0.3 (base fixa em 1.0.0, compara com build_release_1.0.2 antigo)
python tools/make_release.py --version 1.0.3 --make-update --old build_release_1.0.2 --base-dir build_release_1.0.0 --no-build

# Comandos frequentemente usados
# Fluxo completo: compilar + coletar artefatos de release
python tools/make_release.py --version 1.0.1

# Compilar e gerar pacote completo + pacote delta de atualização
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0

# Binários já compilados, apenas empacotar, ignorar etapa de compilação
python tools/make_release.py --version 1.0.1 --no-build

# Gerar pacotes de atualização com notas de versão e sinalizador de atualização forçada
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --update-log "1.Otimizar interface de usuário\n2.Corrigir problemas conhecidos" --force-update
```

### Breve referência de parâmetros

- `--version x.x.x`: Definir número da versão de lançamento
- `--no-build`: Ignorar compilação C++, apenas empacotar (usar quando os binários já estiverem compilados)
- `--make-update`: Habilitar geração de pacote delta; produz pacote completo e patch delta
- `--old <dir>`: Diretório da versão oficial anterior, usado para comparação de arquivos para gerar patch delta
- `--base-dir <dir>`: Diretório de referência global, sempre preencha com `build_release_1.0.0`
- `--update-log`: Notas da versão; use `\n` para quebras de linha
- `--force-update`: Habilitar sinalizador de atualização forçada; o cliente obrigará a atualização

> Os arquivos gerados ficam em `build_release_<version>`. Faça upload dos arquivos zip dessa pasta no GitHub Releases.

## Licença

Licença MIT, consulte o arquivo `LICENSE` na raiz do repositório.

## Apoie o projeto

SnapOCR é totalmente open‑source e gratuito para uso pessoal. Se achar essa ferramenta útil, você pode apoiar o desenvolvimento contínuo voluntariamente pelo Ko‑fi:
👉 [https://ko-fi.com/edenls](https://ko%E2%80%91fi.com/edenls)

## Contato e feedback

‑ E‑mail: [senjin@foxmail.com](mailto:senjin@foxmail.com)
‑ É preferível enviar relatos de bugs e sugestões por meio do GitHub Issues.
